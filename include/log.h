/**
 * @author:MgJun
 * @brief: 日志系统
 * @date：23/3/14
*/
#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>

#include "buffer.h"
#include "blockqueue.h"

class Log{
public:
    void init(int level, const char* path = "./log",
                const char* suffix = ".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char* format, ...);
    void flush();


    //日志等级
    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() {return isOpen_;}
private:
    Log();
    void AppendLevelTitle_(int level);
    virtual ~Log();
    void AsyncWrite_();             //异步写

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;

    int lineCount_;
    int toDay_;

    bool isOpen_;

    Buffer buff_;
    int level_;
    bool isAsync_;

    FILE* fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_;
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...)\
    do{\
        Log* log = Log::Instance();\
        if(log->IsOpen() && log->GetLevel() <= level){\
            log->write(level, format, ##__VA_ARGS__);\
            log->flush();\
        }\
    }while(0);                    //do{} while(0)保证只执行一次并且不用担心分号的问题

#define LOG_DEBUG(format, ...) do{LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do{LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do{LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do{LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

/*这段代码定义了一个简单的日志输出宏，用于在 C++ 代码中输出不同级别的日志信息。
宏定义了四个不同级别的日志输出：LOG_DEBUG、LOG_INFO、LOG_WARN 和 LOG_ERROR，对应输出调试、信息、警告和错误信息。
每个宏调用了一个名为 LOG_BASE 的基础宏，该基础宏会将日志信息写入一个名为 Log 的单例类中，具体输出哪些信息由 Log 类的实现决定。
这段代码的实现中使用了可变参数宏（variadic macro）和 do-while 循环，以避免一些潜在的编译错误。*/

/*
(format, ...)是可变参数列表，可以用来传递不定数量的参数给函数或宏。
在这个代码中，它允许用户在调用宏时，以类似于printf函数的方式，传递不定数量的参数，即格式化字符串和相应的值。
这样就可以在日志输出中动态地插入一些变量，便于观察和调试。*/

/*这里使用宏定义的主要原因是为了让日志记录代码更加简洁和易于维护。通过使用宏定义，可以将日志记录的核心逻辑封装在一个可复用的代码块中，
并且可以轻松地在需要时修改日志级别、输出格式等设置，而不必在每个记录日志的地方都手动添加相同的逻辑。
另外，使用宏定义还可以避免在多处重复定义相同的函数，从而节省了代码量和编译时间*/
