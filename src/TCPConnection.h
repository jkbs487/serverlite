#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <functional>
#include <fcntl.h>
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
    TCPConnection(int epfd, int fd, struct sockaddr_in localAddr, struct sockaddr_in peerAddr): 
        epfd_(epfd), 
        sockfd_(fd), 
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        state_(Connecting), 
        channel_(new Channel(epfd, fd))
    {
        channel_->setRecvCallback(std::bind(&TCPConnection::handleRecv, this));
        channel_->setSendCallback(std::bind(&TCPConnection::handleSend, this));
    }
    ~TCPConnection();
    void send(std::string data);
    void sendFile(std::string filePath);
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
    int sockfd_;
    std::string recvBuf_;
    std::string sendBuf_;
    struct sockaddr_in localAddr_;
    struct sockaddr_in peerAddr_;
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

TCPConnection::~TCPConnection() {
    // 关闭fd
    ::close(sockfd_);
    delete channel_;
}

void TCPConnection::send(std::string data)
{
    if (state_ == Disconnected) {
        printf("disconnected, give up send\n");
        return;
    }
    size_t remaining = data.size();
    ssize_t nsend = 0;

    // 先还要判断是否已经注册写事件，已注册就跳过
    // 还判断写缓存区是否为空，为空才能直接发
    if (!channel_->isSending() && sendBuf_.size() == 0) {
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
            perror("send");
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    return;
                }
            }
        }
    }
    // 还有剩余未发完，注册写事件
    if (remaining > 0) {
        sendBuf_ = data.substr(nsend, remaining);
        channel_->enableSend();
    }
}

void TCPConnection::sendFile(std::string filePath)
{
    const int count = 1024 * 1024;
    int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd < 0) {
        perror("open");
        return;
    }

    struct stat statbuf;
    fstat(fileFd, &statbuf);
    long fileSize = statbuf.st_size; 

    long offset = 0;
    while (offset < fileSize) {
        if (fileSize - offset >= count) {
            ::sendfile64(channel_->fd(), fileFd, &offset, count);
        } else {
            ::sendfile64(channel_->fd(), fileFd, &offset, fileSize - offset);
        }
    }
    printf("file trans complete\n");
}

void TCPConnection::handleRecv()
{
    int fd = channel_->fd();
    char buffer[1024];
    memset(buffer, 0, sizeof buffer);
    ssize_t ret = ::recv(fd, buffer, sizeof buffer, 0);
    recvBuf_.append(std::string(buffer));
    if (ret < 0) {
        if (errno == EINTR) return;
        perror("recv");
        channel_->remove();
        closeCallback_(this);
    } else if (ret == 0) {
        handleClose();
    } else {
        messageCallback_(this, std::move(recvBuf_));
    }
    return;
}

void TCPConnection::handleSend()
{
    if (state_ == Disconnected) {
        printf("disconnected, give up send\n");
        return;
    }
    if (channel_->isSending()) {
        ssize_t nsend = ::send(channel_->fd(), sendBuf_.c_str(), sendBuf_.size(), 0);
        if (nsend > 0) {
            sendBuf_ = sendBuf_.substr(nsend, sendBuf_.size()-nsend);
            // 如果写缓存已经全部发送完毕，取消写事件
            if (sendBuf_.size() == 0) {
                channel_->disableSend();
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
            channel_->disableSend();
        }
    }
    else {
        printf("fd %d is down, no more writing.\n", channel_->fd());
    }
}

void TCPConnection::connectEstablished()
{
    setState(Connected);
    channel_->enableRecv();
    connectionCallback_(this);
}

void TCPConnection::connectDestroyed()
{
    if (state_ == Connected) {
        channel_->disableAll();
        connectionCallback_(this);
    }
    channel_->remove();
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
