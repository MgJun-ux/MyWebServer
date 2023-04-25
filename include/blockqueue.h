/**
 * @author MgJun
 * @brief 阻塞队列,底层实现是互斥量+deque
 * 这段代码实现了一个线程安全的阻塞队列(Blocking Queue)模板类 BlockDeque，支持多线程的入队和出队操作，并且具有阻塞和超时等待的功能。
BlockDeque 是基于 std::deque 实现的，使用 std::mutex 和 std::condition_variable 进行线程同步，保证多线程安全访问。它具有以下功能：
构造函数指定队列容量。
析构函数关闭队列，并清空队列。
清空队列。
判断队列是否为空。
判断队列是否已满。
关闭队列，清空队列。
获取队列长度。
获取队列容量。
获取队列首部元素。
获取队列尾部元素。
向队列尾部插入元素，如果队列已满则阻塞等待。
向队列首部插入元素，如果队列已满则阻塞等待。
从队列中取出首部元素并删除，如果队列为空则阻塞等待。
从队列中取出首部元素并删除，如果队列为空则等待指定时间后返回。如果等待超时则返回 false。如果队列已关闭则返回 false。
 * @date 23/3/13
*/

#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque{
public:
    explicit BlockDeque(size_t MaxCapacity = 1000); //explicit 不让隐式转换

    ~BlockDeque();

    //队列常规操作
    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T& item);

    void push_front(const T& item);

    bool pop(T& item);

    bool pop(T& item, int timeout);

    void flush();   

private:
    std::deque<T> deq_;

    size_t capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity):capacity_(MaxCapacity){
    assert(MaxCapacity > 0);
    isClose_ = false;
};


template<class T>
BlockDeque<T>::~BlockDeque(){
    Close();
};

template<class T> 
void BlockDeque<T>::Close(){
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condConsumer_.notify_all();
    condProducer_.notify_all();
};

template<class T>
void BlockDeque<T>::flush() {           //这段代码的作用是通知一个消费者线程从阻塞状态中唤醒，以便其能够检查队列中是否有可消费的元素。
    condConsumer_.notify_one();
};


template<class T>
void BlockDeque<T>::clear(){
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
};

template<class T>
T BlockDeque<T>::front(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
};

template<class T>
T BlockDeque<T>::back(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
};

template<class T>
size_t BlockDeque<T>::size(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
};

template<class T>
size_t BlockDeque<T>::capacity(){
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
};

/*生产者线程使用push_back()或push_front()方法将元素添加到队列中，
如果队列已满，则线程被阻塞等待空余空间。当生产者线程向队列中添加了新元素时，需要通过调用condConsumer_.notify_one()来通知一个（或所有）等待的消费者线程可以开始消费元素。

消费者线程使用pop()方法从队列中取出元素，
如果队列为空，则线程被阻塞等待元素的到来。当消费者线程从队列中取出一个元素后，需要通过调用condProducer_.notify_one()来通知一个（或所有）等待的生产者线程可以开始生产元素。*/
template<class T>
void BlockDeque<T>::push_back(const T& item){
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_){
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
};

template<class T>
void BlockDeque<T>::push_front(const T& item){
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_){
        condProducer_.wait(locker);
    }
    deq_.push_front();
    condConsumer_.notify_one();
};

template<class T>
bool BlockDeque<T>::empty(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
};

template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size() >= capacity_;
};

template<class T>
bool BlockDeque<T>::pop(T& item){
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        condConsumer_.wait(locker);
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T& item, int timeout){
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout))
        == std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
        item = deq_.front();
        deq_.pop_front();
        condProducer_.notify_one();
        return true;
    }
}
/*
在 push_back() 和 push_front() 方法中使用 unique_lock 是为了在生产者尝试向队列中添加元素时能够对队列进行条件变量等待，
以防队列满，因此需要能够随时释放锁以便其他线程可以修改队列。而其他方法，如 front()、back()、size() 等则只涉及读取队列中的元素，
并且不会被阻塞，因此使用 lock_guard 来保护共享资源即可。lock_guard 适用于保护代码块中的代码，
并且当控制流离开代码块时自动释放锁，因此对于这些只涉及到读取队列的方法，使用 lock_guard 更加方便和高效。*/