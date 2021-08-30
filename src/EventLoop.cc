#include "EventLoop.h"
#include "Channel.h"

#include <thread>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>

EventLoop::EventLoop(): 
    quit_(true),
    threadId_(std::this_thread::get_id()), 
    events_(32)
{
    epfd_ = epoll_create(1);
}

EventLoop::~EventLoop()
{
    ::close(epfd_);
}

void EventLoop::quit()
{
    quit_ = false;
}

void EventLoop::loop()
{
    while (quit_)
    {
        int numEvents = epoll_wait(epfd_, &*events_.begin(), static_cast<int>(events_.size()), -1);
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
    }
}

void EventLoop::createChannel(Channel *channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof event);
    event.data.ptr = channel;
    event.events = channel->events();
    epoll_ctl(epfd_, EPOLL_CTL_ADD, channel->fd(), &event);
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
        epoll_ctl(epfd_, EPOLL_CTL_ADD, channel->fd(), &event);
    } else {
        // 调用 channel->disableAll() 会到这里
        if (channel->isNoneEvent()) {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, channel->fd(), &event);
            channel->setState(Delete);
        }
        else {
            epoll_ctl(epfd_, EPOLL_CTL_MOD, channel->fd(), &event);
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
        epoll_ctl(epfd_, EPOLL_CTL_DEL, channel->fd(), &event);
    }
}