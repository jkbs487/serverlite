#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logger.h"

#include <functional>
#include <unistd.h>

using namespace slite;

EventLoopThread::EventLoopThread(): 
    loop_(nullptr), 
    terminate_(false), 
    mutex_(),
    cond_()
{
}

EventLoopThread::~EventLoopThread()
{
    LOG_DEBUG << "~EventLoopThread";
    if (loop_) {
        terminate_ = true;
        loop_->quit();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_ = std::thread(std::bind(&EventLoopThread::threadFunc, this));
    EventLoop* eventLoop = nullptr;
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [&]{ return loop_ != nullptr; });
    eventLoop = loop_;
    return eventLoop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    LOG_TRACE << "init(): print: pid = " << getpid() << ", tid = " 
        << std::this_thread::get_id() << ", loop = " << &loop;
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