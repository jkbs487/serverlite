#pragma once

#include <functional>
#include <string>

#include <sys/epoll.h>

class Channel 
{
    typedef std::function<void ()> RecvCallback;
    typedef std::function<void ()> SendCallback;

public:
    Channel(int fd): fd_(fd) {

    }
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
    }
    void enableSend() {
        event_ |= SendEvent;
    }
    void disableRecv() {
        event_ &= ~RecvEvent;
    }
    void enableRecv() {
        event_ |= RecvEvent;
    }
    void disableAll() {
        event_ = NoneEvent;
    }
    bool isSending() {
        return event_ & SendEvent;
    }
private:
    static const int SendEvent, RecvEvent, NoneEvent;
    int fd_;
    int event_;
    RecvCallback recvCallback_;
    SendCallback sendCallback_;
};

const int Channel::SendEvent = EPOLLOUT;
const int Channel::RecvEvent = EPOLLIN;
const int Channel::NoneEvent = 0;

void Channel::handleEvent()
{
    if (event_ & EPOLLOUT) sendCallback_();
    else if (event_ & EPOLLIN) recvCallback_();
}