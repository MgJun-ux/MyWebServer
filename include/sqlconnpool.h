/**
 * @author: MgJun
 * @brief: 数据库连接池,linux c++连接mysql: https://www.jianshu.com/p/8229bf719163
 * 这是一个SQL连接池的头文件。该连接池采用单例模式，通过提供一个静态的Instance方法获取连接池对象。
 * 该连接池中维护了一个MYSQL连接的队列，当需要连接数据库时，从队列中取出一个连接，使用完毕后放回队列。连接池中还包含一个信号量，用于控制连接的数量。
 * 其他主要方法包括GetConn、FreeConn、Init和ClosePool等，用于获取连接、释放连接、初始化连接池和关闭连接池
 * @date:23/3/20
*/

#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include <assert.h>
#include "log.h"

class SqlConnPool{
public:
    static SqlConnPool* Instance();


    MYSQL *GetConn();

    void FreeConn(MYSQL* conn);

    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int connSize);

    void ClosePool();


private:

    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;
    int userCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_;

    std::mutex mtx_;
    sem_t semID_; 
};
