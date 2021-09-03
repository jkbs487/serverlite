#pragma once

#include <vector>
#include <unordered_map>
#include <thread>
#include <functional>
#include <memory>
#include <mutex>

class Channel;
struct epoll_event;

class EventLoop
{
public:
    typedef std::function<void()> Functor;
    
    EventLoop();
    ~EventLoop();

    void loop();

    void quit();
    void wakeup();
    void runTask(const Functor &cb);
    void pushTask(const Functor &cb);
    //void 
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }
    static EventLoop *getEventLoopOfCurrentThread();

private:
    void handleRead(); // for wake up
    void doTask();  

    int epollFd_;
    bool quit_;
    bool doingTask_;
    int wakeupFd_;
    std::mutex mutex_;
    std::thread::id threadId_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::vector<struct epoll_event> events_;
    std::unordered_map<int, Channel*> channels_;
    std::vector<Functor> taskQueue_;
};
