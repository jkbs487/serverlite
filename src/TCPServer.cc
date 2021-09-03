#include "TCPServer.h"

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
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
#include <iostream>

void defaultConnectionCallback(const TCPConnectionPtr& conn)
{
    std::cout << inet_ntoa(conn->localAddr().sin_addr) << ":" << conn->localAddr().sin_port 
    << " -> " << inet_ntoa(conn->peerAddr().sin_addr) << ":" << conn->peerAddr().sin_port
    << (conn->connected() ? " UP" : " DOWN") << std::endl;
}

void defaultMessageCallback(const TCPConnectionPtr& conn, std::string buffer)
{
    std::string data;
    data.swap(buffer);
    printf("recv: %s, bytes: %ld\n", data.c_str(), data.size());
}

TCPServer::TCPServer(std::string host, uint16_t port, EventLoop *loop, std::string name): 
    name_(name),
    host_(host), 
    port_(port), 
    eventLoop_(loop),
    threadPool_(new EventLoopThreadPool(eventLoop_)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(&defaultMessageCallback),
    nextConnId_(0)
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
    std::cout << "TCPServer::~TCPServer [" << name_ << "]" << std::endl;
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
    threadPool_->start();
}

void TCPServer::handleAccept() 
{
    int connfd;
    struct sockaddr_in peerAddr;
    ::bzero(&peerAddr, 0);
    socklen_t peerLen = sizeof(peerAddr);

    connfd = accept(acceptChannel_->fd(), reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
    if (connfd == -1) {
        if (errno == EWOULDBLOCK || errno == EINTR) {
            return;
        } else {
            perror("accept");
            acceptChannel_->disableRecv();
            return;
        }
    }

    int flags = fcntl(connfd, F_GETFL, 0); 
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr), &localLen);

    std::string connName = name_ + " - " + host_ + ":" + std::to_string(port_) + "#" + std::to_string(nextConnId_++);

    std::cout << "TCPServer::handleAccept [" << name_ << "] - new connection [" << connName 
    << "] from " << inet_ntoa(peerAddr.sin_addr) << ":" << peerAddr.sin_port << std::endl;

    EventLoop* loop = threadPool_->getNextLoop();
    TCPConnectionPtr conn = std::make_shared<TCPConnection>(loop, connfd, peerAddr, localAddr, connName);

    connections_.push_back(conn);
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));
    loop->runTask(std::bind(&TCPConnection::connectEstablished, conn));
    return;
}

void TCPServer::removeConnection(const TCPConnectionPtr& conn)
{
    eventLoop_->runTask(std::bind(&TCPServer::removeConnectionInLoop, this, conn));
}

void TCPServer::removeConnectionInLoop(const TCPConnectionPtr& conn)
{
    std::cout << "TCPServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name() << std::endl;
    // 这里删除连接，触发连接析构，关闭fd
    auto it = std::find(connections_.begin(), connections_.end(), conn);
    if (it != connections_.end()) {
        connections_.erase(it);
    }
    EventLoop* loop = conn->getLoop();
    loop->runTask(std::bind(&TCPConnection::connectDestroyed, conn));
}

void TCPServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}