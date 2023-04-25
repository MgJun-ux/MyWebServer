/**
 * @author:MgJun
 * @brief:epoll io复用封装这段代码实现了一个 Epoller 类，使用 epoll 来实现 I/O 多路复用。
 * 其中包括了添加、修改和删除文件描述符的函数，以及等待事件的函数。这个类还能返回文件描述符和相应事件的数量。
 * @date:23/3/23
*/

#pragma once

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller{
public:

    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs = -1);

    int GetEventFd(size_t i) const;

    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};