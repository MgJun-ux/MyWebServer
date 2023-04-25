/**
 * @author:MgJun
 * @brief：这个 webserver 类是对整个 web 服务器的抽象，基于 HTTPconnection 类、timer 类、epoller 类、threadpool 类实现一个完整的高性能 web 服务器的所有功能。

需要满足的功能有：

初始化服务器，为 HTTP 的连接做好准备；
处理每一个 HTTP 连接；
用定时器给每一个 HTTP 连接定时，并且处理掉过期的连接；
运用 IO 多路复用技术提升 IO 性能；
运用线程池技术提升服务器性能；

 * @date:23/3/24
*/

#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "log.h"
#include "heaptimer.h"
#include "sqlconnpool.h"
#include "threadpool.h"
#include "sqlconnRAII.h"
#include "httpconn.h"


class WebServer{

public:
    WebServer(
        int port, int trigMode, int timeoutMs, bool OptLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbname, int connPoolNum, int threadNum,
        bool openLog, int LogLevel, int LogQueSize
    );

    ~WebServer();
    void Start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);


    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);


    void OnRead_(HttpConn* client); //这两个函数就是将读写的底层实现函数加入到线程池，两个读写的底层实现函数为：
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);


    int port_;
    bool openLinger_;
    int timeoutMs_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;


    uint32_t listenEvent_;
    uint32_t  connEvent_;


    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};