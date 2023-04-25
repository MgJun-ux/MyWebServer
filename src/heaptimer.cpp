#include "heaptimer.h"


/*这段代码实现了堆中某个节点的上移操作，保证了堆的性质。
具体来说，它将当前节点与其父节点进行比较，如果当前节点比父节点小，则将当前节点上移，直到找到其正确的位置，或者到达堆顶。

其中，变量i表示当前节点的位置，变量j表示其父节点的位置。在while循环中，如果当前节点已经到达堆顶（j < 0），或者当前节点比父节点大，则退出循环。
否则，交换当前节点和父节点的位置，更新i和j，继续向上比较。*/
void HeapTimer::siftup_(size_t i){
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0){
        if(heap_[j] < heap_[i]) break;
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2; 
    } 
}


void HeapTimer::SwapNode_(size_t i, size_t j){
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

/*
这段代码实现了堆的向下调整操作。它的作用是将指定节点index在堆中向下调整，直到满足堆的性质。其中n表示堆的大小。函数返回值为bool类型，表示指定节点是否需要继续向下调整。
具体实现过程如下：
首先，对index和n进行了断言检查，确保它们在有效范围内。
然后，定义了两个变量i和j，分别表示当前节点和它的左儿子。在while循环中，只要当前节点还有左儿子，就继续循环。在循环中，首先比较左右儿子的大小，选出较小的那个。
然后，如果当前节点的值已经比较小，那么就退出循环；否则，交换当前节点和较小儿子的位置，并更新i和j的值。最后，返回i是否大于index的结果，以判断是否需要继续向下调整。*/
bool HeapTimer::siftdown_(size_t index, size_t n){
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());

    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n){
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

/*它提供了一个add方法，用于添加一个定时任务。
如果任务已经存在，add方法会更新任务的超时时间和回调函数，并重新调整堆结构。
如果任务不存在，则会将新任务插入堆尾，并调整堆结构。在这个堆中，最小的元素（即最先超时的任务）位于堆顶，每次取出堆顶元素并执行回调函数，即可实现定时任务的管理。*/
void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb){
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0){
        //新节点，堆尾插入，调整堆
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    }else{
        //已有节点
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size()))
            siftup_(i);
    }
}

void HeapTimer::doWork(int id){
    //删除指定id节点，并触发回调函数
    if(heap_.empty() || ref_.count(id) == 0) return;
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::del_(size_t index){
    //删除指定位置节点
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    //将要删除的节点还到堆尾，然后调整堆
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n){
        SwapNode_(i, n);
        if(!siftdown_(i, n)){
            siftup_(i);
        }
    }
    //删除堆尾
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout){
    //更新指定节点的超时时间
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    siftdown_(ref_[id], heap_.size());
}

void HeapTimer::tick(){
    //清除超时节点
    if(heap_.empty()) return;
    while(!heap_.empty()){
        TimerNode node = heap_.front();
        /*这行代码是计算当前时间与定时器节点过期时间之间的时间差，然后将其转换为毫秒（MS是毫秒的std::chrono::duration类型别名），
        最后检查时间差是否大于0，以判断该定时器节点是否已经过期。如果时间差大于0，表示该定时器节点还未过期，不需要继续处理；
        如果时间差小于等于0，表示该定时器节点已经过期，需要执行回调函数并将其从定时器堆中删除。*/
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) break;
        node.cb();
        pop();
    }
}

void HeapTimer::pop(){
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear(){
    ref_.clear();
    heap_.clear();
}

/*该函数用于获取下一个定时器的超时时间（毫秒）。首先调用 tick() 函数清除已经超时的定时器节点。
然后，如果堆非空，返回堆顶定时器节点的超时时间与当前时间差的毫秒数，
即 std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count()。
如果该值小于 0，说明堆顶节点已经超时，将其设为 0 后返回。如果堆为空，返回 -1*/
int HeapTimer::GetNextTick(){
    tick();
    size_t res = -1;
    if(!heap_.empty()){
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(res < 0) res = 0;
    }
    return res;
}