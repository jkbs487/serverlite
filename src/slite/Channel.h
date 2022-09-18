#pragma once

#include <functional>
#include <memory>

namespace slite
{

class EventLoop;

class Channel
{
    using RecvCallback = std::function<void (int64_t)>;
    using SendCallback = std::function<void ()>;
    using CloseCallback = std::function<void ()>;
    using ErrorCallback = std::function<void ()>;
public:
    enum ChannelState {
        NEW, 
        ADD, 
        DELETE 
    };

    Channel(EventLoop *eventLoop, int fd);
    ~Channel();
    
    Channel(const Channel&) = delete;
    Channel& operator =(const Channel&) = delete;
    
    void handleEvents(int64_t receiveTime);

    void setRecvCallback(const RecvCallback& cb) 
    { recvCallback_ = cb; }
    void setSendCallback(const SendCallback& cb) 
    { sendCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) 
    { closeCallback_ = cb; }
    void setErrorCallback(const ErrorCallback& cb) 
    { errorCallback_ = cb; }
    int fd() { return fd_; }
    int events() 
    { return events_; }
    int state() 
    { return state_; }
    void setRevents(int events) 
    { revents_ = events; }
    void setState(ChannelState state) 
    { state_ = state; }
    void disableSend() 
    { events_ &= ~SendEvent; update(); }
    void enableSend() 
    { events_ |= SendEvent; update(); }
    void disableRecv() 
    { events_ &= ~RecvEvent; update(); }
    void enableRecv() 
    { events_ |= RecvEvent; update(); }
    void disableAll() 
    { events_ = NoneEvent; update(); }
    bool isNoneEvent() 
    { return events_ == NoneEvent; }
    bool isSending() 
    { return events_ & SendEvent; }
    bool isRecving() 
    { return events_ & RecvEvent; }
    void remove();

    // extend self life, prevent destroy self in handleEvent.
    void tie(const std::shared_ptr<void>& owner);
private:
    void update();
    void handleEventsWithGuard(int64_t receiveTime);

    static const int SendEvent, RecvEvent, NoneEvent;

    EventLoop* eventLoop_;
    int fd_;
    int events_;
    int revents_;
    bool tied_;
    bool eventHandling_;
    ChannelState state_;

    std::weak_ptr<void> tie_;

    RecvCallback recvCallback_;
    SendCallback sendCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};

} // namespace tcpserver