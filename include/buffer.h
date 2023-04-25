/**
 * @author:MgJun
 * @date:23/3/13
 * @brief:在这个项目中，客户端连接发来的 HTTP 请求以及回复给客户端所请求的资源，都需要缓冲区的存在。
 * 其实，在操作系统的内核中就有缓冲区的实现，read ()/write () 的调用就离不开缓冲区的支持。
 * 但是，在这里用缓冲区的实现不太方便。所以，在这个项目中实现了一个符合需要的缓冲区结构。
在 C++ 的 STL 库中，vector 容器其实就很适合作为缓冲区。为了能够满足我们的需要，我们以 vector 容器作为底层实体，在它的上面封装自己所需要的方法来实现一个自己的 buffer 缓冲区，
    满足读写的需要。

*/

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <unistd.h> //read(),write()
#include <sys/uio.h> //readv(),readv()
#include <assert.h>
#include <cstring>
class Buffer{
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;
    //缓存区中可写字节数
    size_t WriteableBytes() const;
    //缓存区中可读的字节数
    size_t ReadableBytes() const;
    //缓存区中已经读取的字节数
    size_t PrependableBytes() const;

    //获取当前读指针
    const char* Peek() const;


    void EnsureWriteable(size_t len);
    //更新写指针
    void HasWritten(size_t len);


    //更新读指针的位置
    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);


    //初始化清空buffer
    void RetrieveAll() ;
    //将缓冲区的数据转化为字符串
    std::string RetrieveAllToStr();

    //获取当前写指针
    const char* BeginWriteConst() const;
    char* BeginWrite();
    
    


    ssize_t readFd(int fd, int* Errno);
    ssize_t writeFd(int fd, int* Errno);//与客户端直接IO的读写接口,用read(),write(),readv(),writev()实现

    //处理读写接口
    void Append(const char* str, size_t len);
    void Append(const std::string& str);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buffer);

   


    //test
    void printContent(){
        std::cout<<"pointer location info:"<<readPos_<<" "<<writePos_<<std::endl;
        for(int i = readPos_; i <= writePos_; ++i){
            std::cout<<buffer_[i]<<std::endl;
        }
        std::cout<<std::endl;
    }
private:

    std::vector<char> buffer_; //缓冲区
    std::atomic<std::size_t> readPos_; //用于指示读指针
    std::atomic<std::size_t> writePos_; //用于指示写指针
    //返回指向缓冲区初始位置的指针
    char* BeginPtr_();
    const char* BeginPtr_() const;
    //扩容
    void MakeSpace_(size_t len);
};