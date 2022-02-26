#pragma once

#include "Channel.h"

#include <atomic>
#include <memory>
#include <functional>

namespace slite
{

class EventLoop;

typedef std::function<void()> TimerCallback;

class Timer : public std::enable_shared_from_this<Timer>
{
public:
    Timer(EventLoop* loop);
    ~Timer();
    int addTimer(double time, double interval, TimerCallback cb);

    int sequence()
    { return sequence_; }

private:
    void handleRead();
    void addTimerInLoop(double time, double interval, TimerCallback cb);

    int timerFd_;
    int sequence_;
    EventLoop* loop_;
    std::unique_ptr<Channel> timerChannel_;
    TimerCallback timerCallback_;

    static std::atomic_int32_t s_numCreate;
};

} // namespace slite