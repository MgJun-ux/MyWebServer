/**
 * @author:MgJun
 * @brief:
 * 设置定时器的主要目的是为了清理过期连接，为了方便找到过期连接，首先考虑使用优先队列，按过期时间排序，让过期的排在前面就可以了。
 * 但是这样的话，虽然处理过期连接方便了，当时没法更新一个连接的过期时间。
 *最后，选择一个折中的方法。用 vector 容器存储定时器，然后在这之上实现堆结构，这样各个操作的代价就得到了相对均衡。
 * 
 * 
 * 这这段代码实现了一个基于最小堆的定时器（Heap Timer），支持添加、删除、调整和清空定时器，并能够以最小堆的方式检测并触发超时事件的回调函数。具体实现包括以下函数：

    siftup_：实现堆的上滤操作，用于向堆中添加新元素或调整元素位置。
    SwapNode_：交换堆中两个元素的位置，并更新元素在引用表（ref_）中的位置。
    siftdown_：实现堆的下滤操作，用于删除元素或调整元素位置。
    add：向定时器中添加元素，如果元素不存在则插入新元素，否则更新已有元素的超时时间和回调函数，并调整元素位置。
    doWork：删除指定元素并调用其回调函数。
    del_：删除指定位置的元素。
    adjust：调整指定元素的超时时间，并调整元素位置。
    tick：检查并触发超时事件，循环遍历堆中元素，直到堆为空或堆顶元素未超时。
    pop：删除堆顶元素。
    clear：清空堆和引用表。
    GetNextTick：获取下一个超时时间，先调用 tick 函数清除超时元素，再返回堆顶元素的超时时间差。
 * @date:23/3/14
*/

#pragma once
#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <functional>
#include <arpa/inet.h>

#include "log.h"

/*这段代码定义了几个类型和函数对象：
    TimeoutCallBack 是一个函数对象类型，表示定时器超时时需要执行的回调函数。
    Clock 是 std::chrono 库中的高精度时钟类型，用于获取时间戳。
    MS 是 std::chrono 库中的毫秒类型。
    TimeStamp 是 Clock::time_point 类型的别名，表示一个时间点。*/
using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

/*
    为了实现定时器的功能，我们首先需要辨别每一个 HTTP 连接，每一个 HTTP 连接会有一个独有的描述符（socket），我们可以用这个描述符来标记这个连接，记为 id。
    同时，我们还需要设置每一个 HTTP 连接的过期时间。
    为了后面处理过期连接的方便，我们给每一个定时器里面放置一个回调函数，用来关闭过期连接。
    为了便于定时器结点的比较，主要是后续堆结构的实现方便，我们还需要重载比较运算符。*/
struct TimerNode{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t){
        return expires < t.expires;
    }
};

class HeapTimer{
public:
    HeapTimer() { heap_.reserve(64);}

    ~HeapTimer() {clear();}

    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack& cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:
    void del_(size_t i);

    void siftup_(size_t i);

    bool siftdown_(size_t index, size_t n);

    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;

    /*映射一个fd对应的定时器在heap_中的位置*/
    std::unordered_map<int, size_t> ref_;
};


