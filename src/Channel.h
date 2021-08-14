#pragma once

#include <functional>
#include <string>

#include <sys/epoll.h>

class Channel 
{
    typedef std::function<void ()> RecvCallback;
    typedef std::function<void ()> SendCallback;

public:
    Channel(int epfd, int fd);
    ~Channel();
    void setRecvCallback(const RecvCallback& cb) {
        recvCallback_ = cb;
    }
    void setSendCallback(const SendCallback& cb) {
        sendCallback_ = cb;
    }
    int fd() {
        return fd_;
    }
    void handleEvent();
    void setEvent(int event) {
        event_ = event; 
    };
    void disableSend() {
        event_ &= ~SendEvent;
        update();
    }
    void enableSend() {
        event_ |= SendEvent;
        update();
    }
    void disableRecv() {
        event_ &= ~RecvEvent;
        update();
    }
    void enableRecv() {
        event_ |= RecvEvent;
        update();
    }
    void disableAll() {
        event_ = NoneEvent;
        update();
    }
    bool isSending() {
        return event_ & SendEvent;
    }
    bool isRecving() {
        return event_ & RecvEvent;
    }
    void remove();
private:
    static const int SendEvent, RecvEvent, NoneEvent;
    int epfd_;
    int fd_;
    int event_;
    void update();
    RecvCallback recvCallback_;
    SendCallback sendCallback_;
};

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

Channel::Channel(int epfd, int fd): epfd_(epfd), fd_(fd), event_(NoneEvent) 
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = event_;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, fd_, &event);
}

Channel::~Channel()
{

}

void Channel::update()
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = event_;
    epoll_ctl(epfd_, EPOLL_CTL_MOD, fd_, &event);
}

void Channel::remove()
{
    struct epoll_event event;
    event.data.ptr = this;
    event.events = event_;
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd_, &event);
}

void Channel::handleEvent()
{
    if (event_ & EPOLLOUT) {
        if (sendCallback_) sendCallback_();
    }
    if (event_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (recvCallback_) recvCallback_();
    }
}
