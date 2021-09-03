#include "EventLoop.h"
#include "Channel.h"

#include <thread>
#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>

__thread EventLoop* t_loopInThisThread = 0;

 EventLoop* EventLoop::getEventLoopOfCurrentThread()
 {
     return t_loopInThisThread;
 }

EventLoop::EventLoop(): 
    epollFd_(epoll_create(1)),
    quit_(true),
    doingTask_(false),
    wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
    threadId_(std::this_thread::get_id()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    events_(32)
{
    std::cout << "EventLoop created " << this << " in thread " << threadId_ << std::endl;

    // make shure on loop per thread
    if (t_loopInThisThread)
    {
        std::cout << "Another EventLoop " << t_loopInThisThread
                << " exists in this thread " << threadId_ << std::endl;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setRecvCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableRecv();
}

EventLoop::~EventLoop()
{
    std::cout << "~EventLoop" << std::endl;
    ::close(epollFd_);
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
}

void EventLoop::quit()
{
    quit_ = false;
    if (!doingTask_) {
        wakeup();
    }
}

void EventLoop::loop()
{
    while (quit_)
    {
        int numEvents = epoll_wait(epollFd_, &*events_.begin(), static_cast<int>(events_.size()), -1);
        if (numEvents < 0) {
            perror("epoll_wait");
            break;
        }
        if (numEvents == 0) continue;

        for (int i = 0; i < numEvents; i++) {
            Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events); 
            channel->handleEvents();
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

    if (channel->state() == New || channel->state() == Delete) {
        if (channel->state() == New) {
            channels_[channel->fd()] = channel;
        }
        else {
            channels_.erase(channel->fd());
        }
        channel->setState(Add);
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, channel->fd(), &event);
    } else {
        // 调用 channel->disableAll() 会到这里
        if (channel->isNoneEvent()) {
            epoll_ctl(epollFd_, EPOLL_CTL_DEL, channel->fd(), &event);
            channel->setState(Delete);
        }
        else {
            epoll_ctl(epollFd_, EPOLL_CTL_MOD, channel->fd(), &event);
        }
    }
}

void EventLoop::removeChannel(Channel *channel)
{
    channels_.erase(channel->fd());
    if (channel->state() == Add) {
        struct epoll_event event;
        ::bzero(&event, sizeof event);
        event.data.ptr = channel;
        event.events = channel->events();
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, channel->fd(), &event);
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        perror("read eventfd");
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        perror("read eventfd");
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