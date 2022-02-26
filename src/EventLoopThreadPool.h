#pragma once

#include <memory>
#include <vector>

namespace slite
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start();
    // round-robin
    EventLoop* getNextLoop();
private:
    EventLoop* baseLoop_;   // Tcpserver loop for accept
    int numThreads_;
    int next_;
    bool started_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

} // namespace tcpserver