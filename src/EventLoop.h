#pragma once

#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <memory>
#include <mutex>

struct epoll_event;

namespace tcpserver
{

class Channel;
class Timer;

typedef std::function<void()> TimerCallback;

class EventLoop
{
public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();
    EventLoop(const EventLoop&) = delete;
    void operator=(const EventLoop&) = delete;

    void loop();

    void quit();
    void wakeup();
    void runTask(const Functor &cb);
    void pushTask(const Functor &cb);

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    int runAfter(double delay, TimerCallback cb);
    int runEvery(double interval, TimerCallback cb);
    void cancel(int sequence);

    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }
    void assertInLoopThread();
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
    std::map<int, Channel*> channels_;
    std::vector<Functor> taskQueue_;
    std::map<int, std::shared_ptr<Timer>> timers_;
};

} // namespace tcpserver