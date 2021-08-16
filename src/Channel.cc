#include <sys/epoll.h>
#include <string>

#include "EventLoop.h"
#include "Channel.h"

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

Channel::Channel(EventLoop *eventLoop, int fd)
:   eventLoop_(eventLoop), 
    fd_(fd), 
    events_(NoneEvent), 
    revents_(NoneEvent)
{
}

Channel::~Channel()
{

}

void Channel::create()
{
    eventLoop_->createChannel(this);
}

void Channel::update()
{
    eventLoop_->updateChannel(this);
}

void Channel::remove()
{
    eventLoop_->removeChannel(this);
}

void Channel::handleEvents()
{
    if (revents_ & EPOLLOUT) {
        if (sendCallback_) sendCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (recvCallback_) recvCallback_();
    }
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (EPOLLERR)) {
        if (errorCallback_) errorCallback_();
    }
}
