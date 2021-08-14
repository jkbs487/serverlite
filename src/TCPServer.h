#pragma once

#include <string>
#include <iostream>
#include <functional>
#include <vector>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "TCPConnection.h"

class TCPServer 
{
    typedef std::function<void (TCPConnection*)> ConnectionCallback;
    typedef std::function<void (TCPConnection*, std::string)> MessageCallback;
    typedef std::function<void (TCPConnection*)> WriteCompleteCallback;
public:
    TCPServer(std::string host, uint16_t port);
    ~TCPServer();
    void start();
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }
private:
    int epfd_;
    std::string host_;
    uint16_t port_;
    Channel *acceptChannel_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    struct epoll_event events_[1024];
    void loop();
    void handleAccept();

    void removeConnection(TCPConnection *conn);
    void defaultConnectionCallback(TCPConnection *conn);
    void defaultMessageCallback(TCPConnection *conn, std::string buffer);
};

TCPServer::TCPServer(std::string host, uint16_t port): host_(host), port_(port)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    epfd_ = epoll_create(1);
    acceptChannel_ = new Channel(epfd_, listenfd);
    
    signal(SIGPIPE, SIG_IGN);
    
    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(listenfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        perror("bind");
        exit(0);
    }
    connectionCallback_ = std::bind(&TCPServer::defaultConnectionCallback, this, std::placeholders::_1);
    messageCallback_ = std::bind(&TCPServer::defaultMessageCallback, this, std::placeholders::_1, std::placeholders::_2);
}

TCPServer::~TCPServer()
{
    acceptChannel_->disableAll();
    acceptChannel_->remove();
    delete acceptChannel_;
}

void TCPServer::start()
{
    listen(acceptChannel_->fd(), 5);
    acceptChannel_->setRecvCallback(std::bind(&TCPServer::handleAccept, this));
    acceptChannel_->enableRecv();
    loop();
}

void TCPServer::loop()
{
    for (;;)
    {
        int num_events = epoll_wait(epfd_, events_, 1024, -1);
        if (num_events < 0) {
            perror("epoll_wait");
            break;
        }
        if (num_events == 0) continue;

        for (int i = 0; i < num_events; i++) {
            Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
            if (events_[i].events & EPOLLIN) {
                channel->setEvent(EPOLLIN);
            }
            if (events_[i].events & EPOLLOUT) {
                channel->setEvent(EPOLLOUT);
            }
            channel->handleEvent();
        }
    }
}

void TCPServer::handleAccept() 
{
    struct sockaddr_in peerAddr;
    ::bzero(&peerAddr, 0);
    socklen_t peerLen = sizeof(peerAddr);

    int connfd = accept(acceptChannel_->fd(), reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
    if (connfd < 0) {
        perror("accept");
        ::close(connfd);
    }

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr), &localLen);

    TCPConnection *newConnection = new TCPConnection(epfd_, connfd, peerAddr, localAddr);
    newConnection->setConnectionCallback(connectionCallback_);
    newConnection->setMessageCallback(messageCallback_);
    newConnection->setWriteCompleteCallback(writeCompleteCallback_);
    newConnection->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));
    newConnection->connectEstablished();
    return;
}

void TCPServer::defaultConnectionCallback(TCPConnection *conn)
{
    if (conn->connected())
        printf("UP\n");
    else {
        printf("DOWN\n");
    }
}

void TCPServer::defaultMessageCallback(TCPConnection *conn, std::string buffer)
{
    printf("recv: %s, bytes: %ld\n", buffer.c_str(), buffer.size());
}

void TCPServer::removeConnection(TCPConnection *conn)
{
    // 这里删除连接，触发连接析构，关闭fd
    conn->connectDestroyed();
    delete conn;
}