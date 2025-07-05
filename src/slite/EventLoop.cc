#include "EventLoop.h"
#include "Channel.h"
#include "TCPHandle.h"
#include "Timer.h"
#include "Logger.h"

#include <thread>
#include <cstring>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>

#include <sys/time.h>

using namespace slite;

namespace {

__thread EventLoop* t_loopInThisThread = 0;

int createEventFd()
{
    int wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd < 0) {
        std::cout << "eventfd error" << std::endl; 
        abort();
    }
    return wakeupFd;
}

}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop(): 
    epollFd_(epoll_create(1)),
    quit_(false),
    doingTask_(false),
    wakeupFd_(createEventFd()),
    threadId_(std::this_thread::get_id()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    events_(32)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    ::signal(SIGPIPE, SIG_IGN);
    // make shure one loop per thread
    if (t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                << " exists in this thread " << threadId_;
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setRecvCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableRecv();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << std::this_thread::get_id();
    ::close(epollFd_);
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    t_loopInThisThread = nullptr;
}

void EventLoop::assertInLoopThread()
{
    assert(isInLoopThread());
}

void EventLoop::quit()
{
    quit_ = true;
    if (!doingTask_) {
        wakeup();
    }
}

void EventLoop::loop()
{
    assertInLoopThread();
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";
    while (!quit_) {
        for (auto c: channels_) LOG_TRACE << "channel: " << c.first << " " << c.second;
        int numEvents = epoll_wait(epollFd_, &*events_.begin(), static_cast<int>(events_.size()), 10000);
        if (numEvents < 0) {
            LOG_ERROR << "epoll_wait error: " << strerror(errno);
            break;
        }
        if (numEvents == 0) continue;

        for (int i = 0; i < numEvents; i++) {
            struct timeval tval;
            ::gettimeofday(&tval, NULL);
            int64_t receiveTime = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

            Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events); 
            channel->handleEvents(receiveTime);
        }

        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }

        doTask();
    }
}

void EventLoop::runTask(const Functor &cb)
{
    if (isInLoopThread()) {
        cb();
    } else {
        pushTask(cb);
    }
}

void EventLoop::pushTask(const Functor &cb)
{
    // taskQueue_ is critical section
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push_back(cb);
    }

    // No need to wake up only when doing network events
    if (!isInLoopThread() || doingTask_) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof event);
    event.data.ptr = channel;
    event.events = channel->events();

    if (channel->state() == Channel::NEW || channel->state() == Channel::DELETE) {
        if (channel->state() == Channel::NEW) {
            LOG_TRACE << "new channel: " << channel->fd() << " " << channel;
            assert(channels_.find(channel->fd()) == channels_.end());
            channels_[channel->fd()] = channel;
        }
        else {
            LOG_TRACE << "delete channel: " << channel->fd() << " " << channel;
            assert(channels_.find(channel->fd()) != channels_.end());
        }
        channel->setState(Channel::ADD);
        ::epoll_ctl(epollFd_, EPOLL_CTL_ADD, channel->fd(), &event);
    } else {
        // 调用 channel->disableAll() 会到这里
        if (channel->isNoneEvent()) {
            ::epoll_ctl(epollFd_, EPOLL_CTL_DEL, channel->fd(), &event);
            channel->setState(Channel::DELETE);
        }
        else {
            ::epoll_ctl(epollFd_, EPOLL_CTL_MOD, channel->fd(), &event);
        }
    }
}

void EventLoop::removeChannel(Channel *channel)
{
    LOG_TRACE << "remove channel: " << channel->fd() << " " << channel;
    assert(channels_.find(channel->fd()) != channels_.end());
    channels_.erase(channel->fd());
    if (channel->state() == Channel::ADD) {
        struct epoll_event event;
        ::bzero(&event, sizeof event);
        event.data.ptr = channel;
        event.events = channel->events();
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, channel->fd(), &event);
    }
    channel->setState(Channel::NEW);
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "recv eventfd error";
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n =  write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "send eventfd error";
    }
}

void EventLoop::doTask()
{
    doingTask_ = true;
    
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(taskQueue_);
    }
    for (const Functor& functor : functors) {
        functor();
    }
    
    doingTask_ = false;
}

int EventLoop::runAfter(double delay, TimerCallback cb)
{
    std::shared_ptr<Timer> timer = std::make_shared<Timer>(this);
    int sequence = timer->sequence();
    timer->addTimer(delay, 0, std::move(cb));
    timers_[sequence] = std::move(timer);
    LOG_TRACE << "create Timer " << sequence;
    return sequence;
}

int EventLoop::runEvery(double interval, TimerCallback cb)
{
    std::shared_ptr<Timer> timer = std::make_shared<Timer>(this);
    int sequence = timer->sequence();
    timer->addTimer(interval, interval, std::move(cb));
    timers_[sequence] = std::move(timer);
    return sequence;
}

void EventLoop::cancel(int sequence)
{
    if (timers_.count(sequence) > 0) {
        timers_.erase(sequence);
        LOG_TRACE << "remove Timer " << sequence;
    }
}