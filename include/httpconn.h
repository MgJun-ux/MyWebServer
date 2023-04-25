/**
 * @author:MgJun
 * @brief:这是一个HTTP连接类的实现代码。它包含了一些方法，例如初始化连接、读取数据、写入数据、处理HTTP请求、关闭连接等。
 * 其中，process() 方法用于处理 HTTP 请求，该方法首先初始化一个请求对象，然后解析请求消息。如果解析成功，该方法会初始化一个响应对象并设置响应头和响应内容。
 * 最后，该方法会将响应数据填充到一个缓冲区中，以备写入到套接字。这个类还包含一些成员变量，例如文件描述符、连接地址、读写缓冲区、响应对象和一些常量等。
 * @date 23/3/23
*/

#pragma once
#include <sys/types.h> //读写文件
#include <sys/uio.h> //sockaddr_in
#include <arpa/inet.h> //atoi()
#include <stdlib.h>
#include <error.h>


#include "httprequest.h"
#include "httpresponse.h"
#include "sqlconnRAII.h"
#include "log.h"
#include "buffer.h"


class HttpConn{
public:

    HttpConn();
    ~HttpConn();

    void init(int fd, const sockaddr_in& addr);
    void Close();
    int GetFD() const;
    struct sockaddr_in GetAddr() const;
    const char* GetIP() const;
    int GetPort() const;

    ssize_t read(int* saveError);
    ssize_t write(int* saveError);

    bool process();

    int ToWriteBytes(){ //需要写入的字节数
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool isKeepAlive() const{
        return request_.IsKeepAlive();
    }


    static bool isET; //是不是et模式
    static const char* srcDir;
    static std::atomic<int> userCount;



private:

    int fd_;
    struct sockaddr_in addr_;

    bool isClosed_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;

};