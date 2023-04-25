#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn(){
    fd_ = -1;
    addr_ = { 0 };
    isClosed_ = true; 
}

HttpConn::~HttpConn(){
    Close();
}

void HttpConn::init(int fd, const sockaddr_in& addr){
    assert(fd > 0);
    userCount++;
    fd_ = fd;
    addr_ = addr;
    writeBuff_.RetrieveAll();
    readBuff_.RetrieveAll();
    isClosed_ = false;
    LOG_INFO("Client[%d](%s:%d), userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close(){
    response_.UnmapFile();
    if(isClosed_ == false){
        isClosed_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFD() const{
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const{
    return addr_;
}


const char* HttpConn::GetIP() const{
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const{
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveError){
    ssize_t len = -1;
    do{
        len = readBuff_.readFd(fd_, saveError);
        if(len <= 0){
            break;
        }
    }while(isET);
    return len;
}

/*这段代码是在实现Http服务器的socket写入数据的功能，具体作用是将写缓冲区中的数据使用writev()函数写入socket中，
同时根据写入字节数更新缓冲区中iov_的指针和长度，直到写入完成或缓冲区空间不足。其中，如果缓冲区中第一个iovec中的数据写入完成，则会将其清空。
同时，为了保证高效率，这段代码使用了do-while循环，判断是否是ET模式，以及待写入数据的长度是否超过了一定的阈值（10240字节），以避免出现性能问题。该函数返回写入socket中的字节数。*/
ssize_t HttpConn::write(int* saveError){
    ssize_t len = -1;
    do{
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0){
            *saveError = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0) break; //传输结束
        else if(static_cast<size_t>(len) > iov_[0].iov_len){
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len){
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else{
            iov_[0].iov_base = (uint8_t* )iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
    }while(isET || ToWriteBytes() > 10240);
    return len;
}

/*这段代码是一个HTTP连接处理函数，主要功能如下：

初始化HTTP请求对象request_。

判断读缓冲区readBuff_是否有可读字节，如果没有则返回false。

调用HTTP请求对象request_的解析函数parse()对读缓冲区readBuff_进行解析，解析成功则初始化HTTP响应对象response_，响应状态码为200；否则初始化HTTP响应对象response_，响应状态码为400。

调用HTTP响应对象response_的MakeResponse()函数生成HTTP响应报文，写入写缓冲区writeBuff_。

填充IO向量结构iov_，iov_[0]指向写缓冲区writeBuff_，iov_[1]指向HTTP响应对象response_的文件数据（如果有文件的话）。

返回true表示处理成功，可以进行写操作。


*/
bool HttpConn::process(){
    request_.Init();
    if(readBuff_.ReadableBytes() <= 0){
        return false;
    }
    else if(request_.parse(readBuff_)){
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else{
        
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);

    //响应头

    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    //文件

    if(response_.FileLen() > 0 && response_.File()){
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}
