#include "Timer.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <cmath>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <sys/timerfd.h>

int createTimerFd()
{
    int timerFd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0) {
        std::cout << "timerfd error" << std::endl; 
        abort();
    }
    return timerFd;
}

std::atomic_int32_t Timer::s_numCreate(0);

Timer::Timer(EventLoop* loop):
    timerFd_(createTimerFd()),
    sequence_(++s_numCreate),
    loop_(loop),
    timerChannel_(loop_, timerFd_)
{
    timerChannel_.setRecvCallback(std::bind(&Timer::handleRead, this));
    timerChannel_.enableRecv();
}

Timer::~Timer()
{
    timerChannel_.disableAll();
    timerChannel_.remove();
    ::close(timerFd_);
}

void Timer::handleRead()
{
    uint64_t howmany;
    ssize_t n = ::read(timerFd_, &howmany, sizeof howmany);
    if (n != sizeof howmany) {
        std::cout << "read timerfd error" << std::endl;
    }
    if (timerCallback_) timerCallback_();

    struct itimerspec howlong;
    ::timerfd_gettime(timerFd_, &howlong);
    if (howlong.it_interval.tv_sec == 0 && howlong.it_interval.tv_nsec == 0) {
        loop_->cancel(sequence_);
    }
}

int Timer::addTimer(double time, double interval, TimerCallback cb)
{
    timerChannel_.tie(shared_from_this());
    loop_->runTask(std::bind(&Timer::addTimerInLoop, this, time, interval, cb));
    return sequence_;
}

void Timer::addTimerInLoop(double time, double interval, TimerCallback cb)
{
    loop_->assertInLoopThread();
    if (fabs(time) < 1e-15) 
        time = 0.001;
    timerCallback_ = cb;
    int64_t microseconds = static_cast<int64_t>(time * 1000);
    struct itimerspec howlong;
    ::bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = static_cast<time_t>(microseconds / 1000);
    howlong.it_value.tv_nsec = static_cast<time_t>((microseconds % 1000) * 1000);

    microseconds = static_cast<int64_t>(interval * 1000);
    howlong.it_interval.tv_sec = static_cast<time_t>(microseconds / 1000);
    howlong.it_interval.tv_nsec = static_cast<time_t>((microseconds % 1000) * 1000);

    ::timerfd_settime(timerFd_, 0, &howlong, NULL);
}