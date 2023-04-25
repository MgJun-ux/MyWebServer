#include "webserver.h"

WebServer::WebServer(
        int port, int trigMode, int timeoutMs, bool OptLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbname, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int LogQueSize):
        port_(port), openLinger_(OptLinger), timeoutMs_(timeoutMs), isClose_(false),
        timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
    srcDir_ = getcwd(nullptr, 256);
    //std::cout << srcDir_ << std::endl;
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbname, connPoolNum);
    InitEventMode_(trigMode);
    if(!InitSocket_()) { isClose_ = true; }
   
    if(openLog){
        //std::cout<< LogQueSize << std::endl;
        Log::Instance()->init(logLevel, "./log", ".log", LogQueSize);
        //Log::Instance()->SetLevel(logLevel);
        if(isClose_) { LOG_ERROR("============Server Init Error!============")}
        else{
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer(){
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

/*这段代码是WebServer类的一个函数InitEventMode_，它会根据传入的参数trigMode的值来初始化监听套接字和连接套接字的事件模式。
根据trigMode的值，可能会设置EPOLLET（边缘触发）和EPOLLONESHOT（一次性事件）标志。
在函数中还有一行代码，将HttpConn类的静态变量isET设置为连接套接字事件中是否设置了EPOLLET标志。*/

void WebServer::InitEventMode_(int trigMode){
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch(trigMode)
    {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;

    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}
/*这段代码是Web服务器的主循环。
在循环中，它首先计算出要等待的时间，然后调用 epoll_wait 函数等待事件。一旦有事件到达，它将处理它们，处理方法取决于事件的类型和关联的文件描述符。
如果事件是 EPOLLIN，表示文件描述符可以读取数据，服务器将调用 DealRead_ 函数来读取数据。如果事件是 EPOLLOUT，表示文件描述符可以写入数据，服务器将调用 DealWrite_ 函数来写入数据。
如果事件是 EPOLLRDHUP、EPOLLHUP 或 EPOLLERR，表示与文件描述符关联的连接出现了错误，服务器将关闭连接。否则，它将记录一个错误。
在该代码中，使用&符号是为了判断事件是否包含某个标志。
事件是一个32位的无符号整数，每个标志占用其中的某一位。
因此，使用&运算可以检查事件中是否设置了某个标志。如果结果不为0，表示该事件包含该标志，需要对该事件进行相应的处理。*/
void WebServer::Start(){
    int timeMS = -1;//无事件阻塞
    if(!isClose_) { LOG_INFO("========== Server start ==========");}

    while(!isClose_){
        if(timeoutMs_ > 0){
            timeMS = timer_->GetNextTick();
        }

        int eventCnt = epoller_->Wait(timeMS);

        for(int i = 0; i < eventCnt; i++){
            //处理事件
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            
            if(fd == listenFd_){
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(events & EPOLLIN){
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT){
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }
            else{
                LOG_ERROR("Unexpected event!");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char* info){
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);

    if(ret < 0){
        LOG_WARN("send error to client[%d] error!", fd);

    }

    close(fd);
}

void WebServer::CloseConn_(HttpConn* client){
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFD());
    epoller_->DelFd(client->GetFD());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr){
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMs_ > 0){
        timer_->add(fd, timeoutMs_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }

    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFD());
}

void WebServer::DealListen_(){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listenFd_, (struct sockaddr*)& addr, &len);
        if(fd <= 0) return ;
        else if(HttpConn::userCount >= MAX_FD){
            SendError_(fd, "Server busy!");
            LOG_WARN("Client is full!");
            return;
        }
        AddClient_(fd, addr);
    }while(listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn* client){
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));

}

void WebServer::DealWrite_(HttpConn* client){
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));

}
void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMs_ > 0) { timer_->adjust(client->GetFD(), timeoutMs_); }
}

void WebServer::OnRead_(HttpConn* client){
    assert(client);
    int ret = -1;
    int readError = 0;
    
    ret = client->read(&readError);
    if(ret <= 0 && readError != EAGAIN){
        CloseConn_(client);
        return ;
    }
    OnProcess(client);
}

void WebServer::OnWrite_(HttpConn* client){
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0){
        if(client->isKeepAlive()){
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0){
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFD(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFD(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFD(), connEvent_ | EPOLLIN);
    }
}

bool WebServer::InitSocket_(){
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}


int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}