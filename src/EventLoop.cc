#include "EventLoop.h"

#include "Channel.h"

#include <sys/epoll.h>

EventLoop::EventLoop(): looping_(true), events_(32)
{
    epfd_ = epoll_create(1);
}

EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
    while (looping_)
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
    event.data.ptr = channel;
    event.events = channel->events();
    epoll_ctl(epfd_, EPOLL_CTL_ADD, channel->fd(), &event);
}

void EventLoop::updateChannel(Channel *channel)
{
    struct epoll_event event;
    event.data.ptr = channel;
    event.events = channel->events();
    epoll_ctl(epfd_, EPOLL_CTL_MOD, channel->fd(), &event);
}

void EventLoop::removeChannel(Channel *channel)
{
    struct epoll_event event;
    event.data.ptr = channel;
    event.events = channel->events();
    epoll_ctl(epfd_, EPOLL_CTL_DEL, channel->fd(), &event);
}