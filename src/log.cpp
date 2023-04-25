#include "log.h"
using namespace std;

Log::Log(){
    lineCount_ = 0;
    isAsync_ = false;
    toDay_ = 0;
    writeThread_ = nullptr;
    deque_ = nullptr;
    fp_ = nullptr;
}

Log::~Log(){
    /*这段代码的作用是：
    检查是否有写线程(writeThread_)，并且该线程可以被join，即该线程还没有被join过；
    如果满足条件，就通过循环清空双端队列(deque_)中的元素，即把队列中的所有数据都写入到日志文件中；
    然后关闭队列，停止队列的读写操作；
    最后等待写线程完成任务，即等待写线程结束并退出。
    综上，这段代码的作用是等待写线程完成任务，并确保队列中的所有数据都被写入到日志文件中。*/
    if(writeThread_ && writeThread_->joinable()){         
        while(!deque_->empty()){
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }
    if(fp_){                //关闭文件
        lock_guard<mutex> locker(mtx_);
        flush();            //刷新缓冲区
        fclose(fp_);
    }
}

int Log::GetLevel(){
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level){
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}

void Log::init(int level = 1, const char* path, const char* suffix, int maxQueueCapacity){
    isOpen_ = true;
    level_ = level;
    /*如果最大容量大于0，表示需要异步记录，将isAsync_设置为true，并创建一个BlockDeque类型的队列，
    并通过unique_ptr将其移动到类成员变量deque_中。同时，创建一个新线程，并通过unique_ptr将其移动到类成员变量writeThread_中，并调用该线程的入口函数FlushLogThread。
    如果最大容量等于或小于0，则不需要异步记录，将isAsync_设置为false。
    */
    if(maxQueueCapacity > 0){
        isAsync_ = true;
        if(!deque_){
            /*
            上述代码中，std::move()函数用于将右值（例如临时创建的对象）转移给另一个对象，而不是进行传统的复制操作，这样可以避免不必要的内存拷贝，从而提高程序的效率。
            在代码中，std::unique_ptr类型的对象在移动时，它所管理的指针会被设置为nullptr，以避免悬空指针的出现。因此，通过使用std::move()函数，可以实现对象的移动而非复制。*/
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = move(newDeque);

            std::unique_ptr<std::thread> newThread(new thread(FlushLogThread));
            writeThread_ = move(newThread);
            
        }
    }else{
        isAsync_ = false;
    }
    
    lineCount_ = 0;

    /*
    这段代码获取了当前时间并生成日志文件的文件名，具体解释如下：

    time(nullptr) 获取当前时间（自1970年1月1日以来的秒数）。
    localtime(&timer) 把时间戳转换成当前时区的时间，返回一个指向结构体 tm 的指针。
    struct tm t = *sysTime 将 tm 结构体复制到 t 变量中。
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_) 
    使用 snprintf 函数格式化字符串生成日志文件名，其中 %04d 表示年份占4位，不足位数用0填充；%02d 表示月份和日期占2位，不足位数用0填充；%s 表示后缀名；
    LOG_NAME_LEN 是文件名的最大长度。
    toDay_ = t.tm_mday 把当前日期保存在 toDay_ 变量中。*/
    
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    //std::cout << "debug" << std::endl;
    path_ = path;
    suffix_ = suffix;
    char filename[LOG_NAME_LEN] = {0};
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
        path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    /*这样加一个大括号并没有改变原有的代码逻辑，只是将一段代码用一对大括号括起来，形成了一个代码块（Block），使这段代码形成了一个作用域。
    这样做的目的是为了在这个代码块结束后，自动销毁 lock_guard 对象，从而解锁 mtx_，避免锁的过期时间过长，提高并发性能。*/
    {
        /*这段代码主要是在初始化Log对象时创建日志文件，并初始化缓冲区。具体做法是先清空缓冲区（buff_.RetrieveAll()），
        然后尝试关闭并重新打开日志文件（先通过fclose关闭，再通过fopen重新打开），
        如果打开文件失败则尝试创建文件所在目录，并再次尝试打开文件。如果最终打开文件失败，则通过assert断言抛出异常。lock_guard用于保证多线程情况下对于同一Log对象的初始化操作是线程安全的。*/
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_){
            flush();
            fclose(fp_);
        }

        fp_ = fopen(filename, "a");
        if(fp_ == nullptr){
            mkdir(path, 0777);
            fp_ = fopen(filename, "a");
        }
        assert(fp_ != nullptr);
    }
}

/*这段代码实现了日志的写入功能，包括将日志按照日期和行数分割成不同的文件，以及将日志写入到文件或异步队列中。
其中，使用了线程安全的缓冲区和锁机制来保证并发写入的正确性。
函数接收一个可变参数，将格式化后的字符串写入缓冲区或异步队列中，并在特定条件下将缓冲区或异步队列中的日志写入到文件中*/
/*buff_ 是一个缓冲区，用于存储日志信息，通过调用 Append 函数来将日志信息追加到缓冲区中，然后再将缓冲区的内容写入文件或放入异步队列中。
缓冲区的作用在于减少写入文件的次数，提高写入效率。在 write 函数中，通过调用 buff_.RetrieveAll() 函数清空缓冲区，将缓冲区中的内容写入文件或放入异步队列中。*/
void Log::write(int level, const char* format, ...){
    struct timeval now = {0, 0};
    /*
    原型
    struct timeval
    {
    __time_t tv_sec;        //Seconds. 
    __suseconds_t tv_usec;  //Microseconds. 
    };
    */
    gettimeofday(&now, nullptr);               //int gettimeofday(struct  timeval*tv,struct  timezone *tz )
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    //这段代码的意思是如果当前日期不等于记录的日期，或者日志行数达到了最大值，则需要进行日志文件的切换。
    if(toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))){
        unique_lock<mutex> locker(mtx_);
        locker.unlock();
        /*在这里解锁操作的目的是为了减小锁的粒度，避免对锁的占用时间过长。
        如果不解锁，那么在修改日志文件名、清空计数器等操作时都需要持有锁，这样会对整个写日志的过程造成较大的阻塞，影响程序的性能。
        因此，在这里先解锁，等到需要重新持有锁时再进行加锁操作，以达到减小锁粒度的目的。*/
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if(toDay_ != t.tm_mday){
            snprintf(newFile, LOG_NAME_LEN - 72, "%s%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }else{
            snprintf(newFile, LOG_NAME_LEN - 72, "%s%s-%d%s", path_, tail, (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }
    /*这段代码的作用是将格式化的日志消息写入缓冲区中，然后将缓冲区的内容写入日志文件或添加到异步队列中。具体地：

    使用snprintf将时间信息和日志级别信息写入缓冲区。
    使用vsnprintf将格式化后的日志消息写入缓冲区。
    将换行符和字符串终止符添加到缓冲区中。
    如果是异步模式且异步队列未满，则将缓冲区的内容添加到异步队列中。
    如果不是异步模式或异步队列已满，则将缓冲区的内容写入日志文件中。
    最后清空缓冲区中的数据。*/
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",     //返回写入字符数组的字符数
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec, now.tv_sec);

        buff_.HasWritten(n);
        AppendLevelTitle_(level);

        /*这是可变参数函数的用法。在这个例子中，format 是一个格式化字符串，它接受不定数量的参数。
        函数调用中的参数列表从 format 后面开始，因此我们需要使用 va_start() 宏来告诉程序从哪里开始读取这些参数，va_end() 宏告诉程序何时停止读取。
        在这个函数中，vsnprintf() 函数接受可变参数列表，将其格式化为一个字符串，并将其写入日志缓冲区。*/
        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WriteableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        if(isAsync_ && deque_ && !deque_->full()){
            deque_->push_back(buff_.RetrieveAllToStr());
        }else{
            fputs(buff_.Peek(), fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLevelTitle_(int level){
    switch(level){
        case(0):
            buff_.Append("[debug]: ", 9);
            break;
        case(1):
            buff_.Append("[info] : ", 9);
            break;
        case(2):
            buff_.Append("[warn] : ", 9);
            break;
        case(3):
            buff_.Append("[error]: ", 9);
            break;
        default:
            buff_.Append("[info] : ", 9);
            break;
    }
}

void Log::flush(){
    if(isAsync_){
        deque_->flush();
    }
    fflush(fp_);
}
/*这段代码实现了一个异步写日志的功能。Log类实现了一个静态函数Instance()，返回Log类的单例实例。
在FlushLogThread()函数中，调用Instance()获取Log类的实例，并调用它的AsyncWrite_()函数进行异步写入。
AsyncWrite_()函数中，从阻塞队列中取出待写入的日志字符串，并通过fputs函数将其写入文件中。由于异步写入的原因，日志的写入与主线程的执行是异步的，不会影响主线程的性能表现。*/
void Log::AsyncWrite_(){
    string str = "";
    while(deque_->pop(str)){
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance(){
    static Log inst;
    return &inst;
}

void Log::FlushLogThread(){
    Log::Instance()->AsyncWrite_(); 
}

