#pragma once

#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>

namespace tcpserver
{

class EventLoop;

class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool terminate_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

} // namespace tcpserver