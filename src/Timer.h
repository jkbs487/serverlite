#pragma once

#include <functional>

class Channel;
class EventLoop;

typedef std::function<void()> TimerCallback;

class Timer
{
public:
    Timer(EventLoop* loop);
    ~Timer();
    void addTimer(double time, double interval);
    void setTimerCallback(TimerCallback cb) { timerCallback_ = cb; }

private:
    void handleRead();
    void addTimerInLoop(double time, double interval);

    int timerFd_;
    EventLoop* loop_;
    Channel* timerChannel_;
    TimerCallback timerCallback_;
};
