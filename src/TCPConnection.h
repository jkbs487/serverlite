#pragma once
#include <functional>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "Channel.h"

class TCPConnection
{
    typedef std::function<void (TCPConnection*)> ConnectionCallback;
    typedef std::function<void (TCPConnection*, std::string)> MessageCallback;
    typedef std::function<void (TCPConnection*)> CloseCallback;
    typedef std::function<void (TCPConnection*)> WriteCompleteCallback;
public:
    TCPConnection(int fd, int epfd): epfd_(epfd), channel_(new Channel(fd)) {
        channel_->setRecvCallback(std::bind(&TCPConnection::handleRecv, this));
        channel_->setSendCallback(std::bind(&TCPConnection::handleSend, this));
    }
    void send(std::string data);
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }
    void handleClose();
    void handleError();
    void connectEstablished();
    void connectDestroyed();
    bool connected() {
        return state_ == Connected;
    }
    void shutdown();

private:
    enum ConnState { Disconnected, Connecting, Connected, Disconnecting };
    int epfd_;
    std::string recv_buf_;
    std::string send_buf_;
    ConnState state_;
    Channel *channel_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    void handleRecv();
    void handleSend();
    void setState(ConnState state) {
        state_ = state;
    }
};

void TCPConnection::send(std::string data)
{
    if (state_ == Disconnected) {
        printf("disconnected, give up send\n");
    }
    size_t remaining = data.size();
    ssize_t nsend = 0;

    // 先还要判断是否已经注册写事件，已注册就跳过
    // 还判断写缓存区是否为空，为空才能直接发
    if (!channel_->isSending() && send_buf_.size() == 0) {
        nsend = ::send(channel_->fd(), data.c_str(), data.size(), 0);
        if (nsend >= 0) {
            remaining = data.size() - nsend; 
            if (remaining == 0 && writeCompleteCallback_) {
                // 如果同步调用，有无限递归风险
                writeCompleteCallback_(this);
            }
        }
        else {
            nsend = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    return;
                }
            }
        }
    }
    // 还有剩余未发完，注册写事件
    if (remaining > 0) {
        send_buf_ = data.substr(nsend, remaining);
        channel_->enableSend();
        struct epoll_event event;
        event.data.ptr = channel_;
        event.events = EPOLLOUT;
        epoll_ctl(epfd_, EPOLL_CTL_MOD, channel_->fd(), &event);
    }
}

void TCPConnection::handleRecv()
{
    struct epoll_event event; 
    int fd = channel_->fd();
    char buffer[1024];
    memset(buffer, 0, sizeof buffer);
    ssize_t ret = ::recv(fd, buffer, sizeof buffer, 0);
    recv_buf_.append(std::string(buffer));
    if (ret < 0) {
        if (errno == EINTR) return;
        perror("recv");
        event.events = EPOLLIN;
        event.data.ptr = channel_;
        epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &event);
        close(fd);
    } else if (ret == 0) {
        handleClose();
    } else {
        messageCallback_(this, std::move(recv_buf_));
    }
    return;
}

void TCPConnection::handleSend()
{
    if (state_ == Disconnected) {
        printf("disconnected, give up send\n");
    }
    ssize_t nsend = ::send(channel_->fd(), send_buf_.c_str(), send_buf_.size(), 0);
    if (nsend > 0) {
        send_buf_ = send_buf_.substr(nsend, send_buf_.size()-nsend);
        // 如果写缓存已经全部发送完毕，取消写事件
        if (send_buf_.size() == 0) {
            channel_->disableSend();
            struct epoll_event event;
            event.data.ptr = channel_;
            event.events = EPOLLIN;
            epoll_ctl(epfd_, EPOLL_CTL_MOD, channel_->fd(), &event);

            if (writeCompleteCallback_) {
                writeCompleteCallback_(this);
            }
        }
        // 是否半关闭状态 
        if (state_ == Disconnecting) {
            shutdown();
        }
    } else {
        printf("handle send error\n");
        return;
    }
    return;
}

void TCPConnection::connectEstablished()
{
    setState(Connected);
    struct epoll_event event;
    event.data.ptr = channel_;
    event.events = EPOLLIN;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, channel_->fd(), &event);
    connectionCallback_(this);
}

void TCPConnection::connectDestroyed()
{
    struct epoll_event event;
    event.data.ptr = channel_;
    event.events = EPOLLIN;
    epoll_ctl(epfd_, EPOLL_CTL_DEL, channel_->fd(), &event);
}

void TCPConnection::handleClose()
{
    setState(Disconnected);
    channel_->disableAll();
    connectionCallback_(this);
    closeCallback_(this);
}

void TCPConnection::shutdown()
{
    if (state_ == Connected) {
        setState(Disconnecting);
        ::shutdown(channel_->fd(), SHUT_WR);
    }
}