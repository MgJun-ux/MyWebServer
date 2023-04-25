#include "epoller.h"


Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent){
    assert(epollFd_ >= 0 && events_.size() > 0);
}


Epoller::~Epoller(){
    close(epollFd_);
}

/*这段代码定义了一个Epoller类的成员函数AddFd，用于往epoll实例中添加一个文件描述符以及相应的事件。
如果文件描述符小于0，则返回false。
否则，定义一个epoll_event类型的变量ev，将其文件描述符和事件赋值为参数传入的值。
最后，调用epoll_ctl函数将该文件描述符添加到epoll实例中，并返回该函数的返回值是否等于0。*/
bool Epoller::AddFd(int fd, uint32_t events){
    if(fd < 0) return false;
    epoll_event ev = { 0 };
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events){
    if(fd < 0) return false;
    epoll_event ev = { 0 };
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd){
    if(fd < 0) return false;
    epoll_event ev = { 0 };
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeoutMs){
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}