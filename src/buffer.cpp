#include "buffer.h"

Buffer::Buffer(int initBuffSize):buffer_(initBuffSize), readPos_(0), writePos_(0){}

size_t Buffer::ReadableBytes() const{
    return writePos_ - readPos_;
}

size_t Buffer::WriteableBytes() const{
    return buffer_.size() - writePos_;
}

size_t Buffer::PrependableBytes() const{
    return readPos_;
}

const char* Buffer::Peek() const{
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWriteable(size_t len){
    if(WriteableBytes() < len)
        MakeSpace_(len);
    assert(WriteableBytes() >= len);
}

void Buffer::HasWritten(size_t len){
     writePos_ += len;
}

void Buffer::Retrieve(size_t len){
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll(){
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr(){
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}


//接受数据用，将接受的数据存入缓冲区
void Buffer::Append(const std::string& str){
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len){
    assert(data);
    Append(static_cast<const char*>(data), len);
}

//Append底层,将str加入当前buffer_
void Buffer::Append(const char* str, size_t len){
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

//发送数据时用，把缓冲区的数据发送
void Buffer::Append(const Buffer& buff){
    Append(buff.Peek(), ReadableBytes());
}


const char* Buffer::BeginWriteConst() const{
    return BeginPtr_() + writePos_;
}

//返回可以写数据的起始位置
char* Buffer::BeginWrite(){
    return BeginPtr_() + writePos_;
}

char* Buffer::BeginPtr_(){
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const{
    return &*buffer_.begin();
}
//将数据从文件中读到分散的内存中
ssize_t Buffer::readFd(int fd, int* saveErrno){
    char buff[65535];
    struct iovec iov[2];
    const size_t writeable = WriteableBytes();
    /*分散读，保证数据全部读完，参考Linux高性能服务器第六章高级io*/
    //iovec第一个成员是内存起始地址，第二个成员是这块内存的长度
    iov[0].iov_base =  BeginPtr_() + writePos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    const ssize_t len = readv(fd, iov, 2);//返回成功读入的字节数
    if(len < 0) 
        *saveErrno = errno;
    else if(static_cast<size_t>(len) <= writeable) //buffer_大小够用，将写指针置后
        writePos_ += len;
    else{                                           //buffer_放不下，剩下的内容放入buff数组，然后将buff数组加入到buffer_中
        writePos_ = buffer_.size();
        Append(buff, len - writeable);
    }
    return len;
}

//将内存即buffer_中数据写入文件
ssize_t Buffer::writeFd(int fd, int* saveErrno){
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0){
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}



void Buffer::MakeSpace_(size_t len){
        //如果buffer_里面剩余的空间有len就进行调整，否则需要申请空间。
    //剩余空间包括write指针之前的空间和可写的空间
    if(WriteableBytes() + PrependableBytes() < len){
        buffer_.resize(writePos_ + len +1);
    }else{
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}