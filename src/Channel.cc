#include <sys/epoll.h>
#include <string>
#include <assert.h>

#include "EventLoop.h"
#include "Channel.h"

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

Channel::Channel(EventLoop *eventLoop, int fd)
:   eventLoop_(eventLoop), 
    fd_(fd), 
    events_(NoneEvent), 
    revents_(NoneEvent),
    state_(New)
{
}

Channel::~Channel()
{

}

void Channel::update()
{
    eventLoop_->updateChannel(this);
}

void Channel::remove()
{
    assert(isNoneEvent());
    eventLoop_->removeChannel(this);
}

void Channel::handleEvents()
{
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        printf("Handle Close\n");
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (EPOLLERR)) {
        printf("Handle Error\n");
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        printf("Handle Read\n");
        if (recvCallback_) recvCallback_();
    }
    if (revents_ & EPOLLOUT) {
        printf("Handle Write\n");
        if (sendCallback_) sendCallback_();
    }
}
