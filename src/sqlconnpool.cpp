#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool(){
    userCount_ = 0;
    freeCount_ = 0;
}

SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool connpool;
    return &connpool;
} 
/*这是一个SQL连接池的成员函数Init。
该函数用于初始化连接池，根据指定的参数创建MYSQL连接，并将其加入到连接池中。
该函数的参数包括数据库主机地址、端口号、用户名、密码、数据库名和连接池的大小。该函数首先通过断言（assert）判断连接池的大小是否大于0，然后循环创建指定数量的MYSQL连接。
对于每个MYSQL连接，函数首先通过mysql_init函数进行初始化，并对返回值进行检查，如果返回NULL，则表示初始化失败，需要记录日志并断言退出。
然后通过mysql_real_connect函数进行连接，参数包括数据库主机地址、端口号、用户名、密码、数据库名等，同样需要对返回值进行检查，如果返回NULL，则表示连接失败，需要记录日志。
最后，将连接加入到连接池的队列中，同时记录连接池中最大连接数和信号量的初始值*/
void SqlConnPool::Init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int connSize = 10){
    assert(connSize > 0);
    for(int i = 0; i < connSize; ++i){
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if(!sql){
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if(!sql){
            LOG_ERROR("MySql Connect error!");
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semID_, 0, MAX_CONN_);/*这行代码是在初始化信号量的值，使用了POSIX信号量库中的sem_init函数。
    sem_init函数的第一个参数为指向要初始化的信号量的指针，第二个参数指定信号量是否应该在进程间共享，如果设置为0，则信号量只能在同一进程的不同线程之间共享，第三个参数指定信号量的初始值。
    在这里，信号量的初值被初始化为连接池的最大连接数，这表示连接池中最多可以有MAX_CONN_个连接同时被使用。*/
}

/*这是一个SQL连接池的成员函数GetConn，用于获取一个可用的MYSQL连接。该函数首先定义一个MYSQL指针变量，并初始化为空。
然后，如果连接池中没有可用的连接，函数将记录一个警告日志并返回空指针。如果连接池中有可用的连接，则会阻塞等待信号量，信号量的值会减1。
这是为了确保当前使用的连接数不超过连接池中的最大连接数。在等待到信号量后，函数通过获取互斥锁来保护对连接池的访问，然后从队列的前端取出一个MYSQL连接，并将其从队列中删除。
最后，函数返回该MYSQL连接的指针。需要注意的是，使用完连接后，必须通过调用FreeConn函数将其释放，否则将无法使用该连接。*/
MYSQL* SqlConnPool::GetConn(){
    MYSQL *sql = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy");
        return nullptr;
    }
    sem_wait(&semID_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

/*是SQL连接池的FreeConn函数，用于将不再使用的MYSQL连接释放回连接池。该函数首先使用断言确保传入的MYSQL指针不为空。
然后，函数获取互斥锁来保护对连接池的访问，将传入的MYSQL连接放回连接池的队列中，并通过调用sem_post函数将信号量的值加1。
这表示连接池中又有一个可用连接。函数执行完后，该连接将可以被其他线程使用。
*/
void SqlConnPool::FreeConn(MYSQL *sql){
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semID_);
}

/*这是一个SQL连接池的成员函数ClosePool。该函数用于关闭连接池，释放连接池中所有的MYSQL连接资源。
该函数首先通过锁保护，遍历连接队列，将队列中的连接逐个关闭，然后通过mysql_library_end()函数结束MYSQL库的使用。
需要注意的是，在使用完MYSQL连接池后，必须调用ClosePool函数来释放资源，否则会造成资源泄漏。*/

void SqlConnPool::ClosePool(){
    lock_guard<mutex> locker(mtx_);
    while(!connQue_.empty()){
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount(){
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool(){
    ClosePool();
}


