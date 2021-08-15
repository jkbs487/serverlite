#pragma once

#include <functional>
#include <string>

#include <sys/epoll.h>

class Channel 
{
    typedef std::function<void ()> RecvCallback;
    typedef std::function<void ()> SendCallback;
    typedef std::function<void ()> CloseCallback;
    typedef std::function<void ()> ErrorCallback;

public:
    Channel(int epfd, int fd);
    ~Channel();
    void setRecvCallback(const RecvCallback& cb) {
        recvCallback_ = cb;
    }
    void setSendCallback(const SendCallback& cb) {
        sendCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    void setErrorCallback(const ErrorCallback& cb) {
        errorCallback_ = cb;
    }
    int fd() {
        return fd_;
    }
    void handleEvents();
    int events() {
        return events_;
    }
    void setRevents(int events) {
        revents_ = events; 
    };
    void disableSend() {
        events_ &= ~SendEvent;
        update();
    }
    void enableSend() {
        events_ |= SendEvent;
        update();
    }
    void disableRecv() {
        events_ &= ~RecvEvent;
        update();
    }
    void enableRecv() {
        events_ |= RecvEvent;
        update();
    }
    void disableAll() {
        events_ = NoneEvent;
        update();
    }
    bool isSending() {
        return events_ & SendEvent;
    }
    bool isRecving() {
        return events_ & RecvEvent;
    }
    void remove();
private:
    static const int SendEvent, RecvEvent, NoneEvent;
    int epfd_;
    int fd_;
    int events_;
    int revents_;
    void update();
    RecvCallback recvCallback_;
    SendCallback sendCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

Channel::Channel(int epfd, int fd): epfd_(epfd), fd_(fd), events_(NoneEvent), revents_(NoneEvent)
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = events_;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, fd_, &event);
}

Channel::~Channel()
{

}

void Channel::update()
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = events_;
    epoll_ctl(epfd_, EPOLL_CTL_MOD, fd_, &event);
}

void Channel::remove()
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = events_;
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd_, &event);
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
