#pragma once

#include "Channel.h"

#include <atomic>
#include <functional>

class EventLoop;

typedef std::function<void()> TimerCallback;

class Timer
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
    Channel timerChannel_;
    TimerCallback timerCallback_;

    static std::atomic_int32_t s_numCreate;
};
