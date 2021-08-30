#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>

EventLoopThread::EventLoopThread(): 
    loop_(nullptr), 
    terminate_(false), 
    mutex_(),
    cond_()
{
    
}

EventLoopThread::~EventLoopThread()
{
    if (loop_) {
        terminate_ = true;
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_ = std::thread(std::mem_fn(&EventLoopThread::threadFunc), this);
    EventLoop* eventLoop = nullptr;
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [&]{ return loop_; });
    eventLoop = loop_;
    return eventLoop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}