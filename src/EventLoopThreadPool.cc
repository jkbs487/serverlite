#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <assert.h>
#include <unistd.h>

using namespace tcpserver;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop):
    baseLoop_(baseLoop),
    numThreads_(0),
    next_(0),
    started_(false)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start()
{
    assert(!started_);

    started_ = true;

    for (int i = 0; i < numThreads_; i++) {
        EventLoopThread* eventLoopThread = new EventLoopThread();
        threads_.push_back(std::unique_ptr<EventLoopThread>(eventLoopThread));
        loops_.push_back(eventLoopThread->startLoop());
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = baseLoop_;
    
    if (!loops_.empty()) {
        if (next_ == numThreads_) next_ = 0;
        loop = loops_.at(next_++);
    }
    return loop;
}