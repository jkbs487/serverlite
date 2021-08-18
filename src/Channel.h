#pragma once

#include <functional>

class EventLoop;

enum ChannelState { New, Add, Delete };

class Channel 
{
    typedef std::function<void ()> RecvCallback;
    typedef std::function<void ()> SendCallback;
    typedef std::function<void ()> CloseCallback;
    typedef std::function<void ()> ErrorCallback;
public:
    Channel(EventLoop *eventLoop, int fd);
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
    int state() {
        return state_;
    }
    void setRevents(int events) {
        revents_ = events; 
    }
    void setState(ChannelState state) {
        state_ = state;
    }
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
    bool isNoneEvent() {
        return events_ == NoneEvent;
    }
    bool isSending() {
        return events_ & SendEvent;
    }
    bool isRecving() {
        return events_ & RecvEvent;
    }
    void remove();
private:
    void update();
    
    static const int SendEvent, RecvEvent, NoneEvent;

    EventLoop* eventLoop_;
    int fd_;
    int events_;
    int revents_;
    ChannelState state_;
    RecvCallback recvCallback_;
    SendCallback sendCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};