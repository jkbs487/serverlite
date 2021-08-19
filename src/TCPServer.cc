#include "TCPServer.h"

#include "EventLoop.h"
#include "Channel.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <algorithm>

TCPServer::TCPServer(std::string host, uint16_t port): 
    host_(host), 
    port_(port), 
    eventLoop_(new EventLoop()),
    connectionCallback_(std::bind(&TCPServer::defaultConnectionCallback, this, std::placeholders::_1)),
    messageCallback_(std::bind(&TCPServer::defaultMessageCallback, this, std::placeholders::_1, std::placeholders::_2))
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    acceptChannel_ = new Channel(eventLoop_, listenfd);
    
    signal(SIGPIPE, SIG_IGN);
    
    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    int flags = fcntl(listenfd, F_GETFL, 0); 
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(listenfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        perror("bind");
        exit(-1);
    }
}

TCPServer::TCPServer(std::string host, uint16_t port, EventLoop *eventLoop): 
    host_(host), 
    port_(port), 
    eventLoop_(eventLoop),
    connectionCallback_(std::bind(&TCPServer::defaultConnectionCallback, this, std::placeholders::_1)),
    messageCallback_(std::bind(&TCPServer::defaultMessageCallback, this, std::placeholders::_1, std::placeholders::_2))
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    acceptChannel_ = new Channel(eventLoop_, listenfd);
    
    signal(SIGPIPE, SIG_IGN);
    
    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    int flags = fcntl(listenfd, F_GETFL, 0); 
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(listenfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        perror("bind");
        exit(-1);
    }
}

TCPServer::~TCPServer()
{
    acceptChannel_->disableAll();
    acceptChannel_->remove();
    delete acceptChannel_;
    delete eventLoop_;
}

void TCPServer::start()
{
    listen(acceptChannel_->fd(), 5);
    acceptChannel_->setRecvCallback(std::bind(&TCPServer::handleAccept, this));
    acceptChannel_->enableRecv();
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

    int flags = fcntl(connfd, F_GETFL, 0); 
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr), &localLen);

    TCPConnectionPtr conn = std::make_shared<TCPConnection>(eventLoop_, connfd, peerAddr, localAddr);

    connections_.push_back(conn);
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));
    conn->connectEstablished();
    return;
}

void TCPServer::defaultConnectionCallback(const TCPConnectionPtr& conn)
{
    if (conn->connected())
        printf("UP\n");
    else {
        printf("DOWN\n");
    }
}

void TCPServer::defaultMessageCallback(const TCPConnectionPtr& conn, std::string buffer)
{
    printf("recv: %s, bytes: %ld\n", buffer.c_str(), buffer.size());
}

void TCPServer::removeConnection(const TCPConnectionPtr& conn)
{
    // 这里删除连接，触发连接析构，关闭fd
    auto it = std::find(connections_.begin(), connections_.end(), conn);
    if (it != connections_.end()) {
        connections_.erase(it);
    }
    conn->connectDestroyed();
}