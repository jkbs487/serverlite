#include <sys/epoll.h>
#include <string>
#include <assert.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

using namespace tcpserver;

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

Channel::Channel(EventLoop *eventLoop, int fd)
:   eventLoop_(eventLoop), 
    fd_(fd), 
    events_(NoneEvent), 
    revents_(NoneEvent),
    tied_(false),
    eventHandling_(false),
    state_(New)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::tie(const std::shared_ptr<void>& owner)
{
    tie_ = owner;
    tied_ = true;
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
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            LOG_TRACE << "[6]TCPConnection use count: " << guard.use_count();
            handleEventsWithGuard();
            LOG_TRACE << "[12]TCPConnection use count: " << guard.use_count();
        }
    }
    else {
        handleEventsWithGuard();
    }
} 

void Channel::handleEventsWithGuard()
{
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (EPOLLERR)) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (recvCallback_) recvCallback_();
    }
    if (revents_ & EPOLLOUT) {
        if (sendCallback_) sendCallback_();
    }
    eventHandling_ = false;
}
