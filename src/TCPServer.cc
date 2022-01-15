#include "TCPServer.h"

#include "Logger.h"
#include "Channel.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <algorithm>

using namespace tcpserver;

TCPServer::TCPServer(std::string host, uint16_t port, EventLoop *loop, std::string name): 
    name_(name),
    host_(host), 
    port_(port),
    loop_(loop),
    acceptor_(new Acceptor(host, port, loop)),
    threadPool_(new EventLoopThreadPool(loop)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(&defaultMessageCallback),
    nextConnId_(0)
{
    acceptor_->setNewConnectionCallback(
        std::bind(&TCPServer::newConnection, this, std::placeholders::_1));
}

TCPServer::~TCPServer()
{
    LOG_DEBUG << "TCPServer::~TCPServer [" << name_ << "]";
    for (TCPConnectionPtr& conn : connections_) {
        conn.reset();
        conn->getLoop()->runTask(std::bind(&TCPConnection::connectDestroyed, conn));
    }
}

void TCPServer::start()
{
    threadPool_->start();
    loop_->runTask(std::bind(&Acceptor::listen, acceptor_.get()));
}

void TCPServer::newConnection(int connfd) 
{
    int flags = fcntl(connfd, F_GETFL, 0); 
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    ::getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr), &localLen);

    struct sockaddr_in peerAddr;
    socklen_t peerLen = sizeof(peerAddr);
    ::getpeername(connfd, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);

    std::string connName;
    connName = name_ + "-" + host_ + ":" + std::to_string(port_) + "#" + std::to_string(nextConnId_++);

    LOG_DEBUG << "TCPServer::handleAccept [" << name_ << "] - new connection [" << connName 
    << "] from " << inet_ntoa(peerAddr.sin_addr) << ":" << peerAddr.sin_port;

    EventLoop* loop = threadPool_->getNextLoop();
    TCPConnectionPtr conn = std::make_shared<TCPConnection>(loop, connfd, localAddr, peerAddr, connName);

    LOG_TRACE << "[1]TCPConnection use count: " << conn.use_count();
    connections_.push_back(conn);
    LOG_TRACE << "[2]TCPConnection use count: " << conn.use_count();
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));
    loop->runTask(std::bind(&TCPConnection::connectEstablished, conn));
    LOG_TRACE << "[5]TCPConnection use count: " << conn.use_count();
    return;
}

void TCPServer::removeConnection(const TCPConnectionPtr& conn)
{
    // 确保在主线程中移除连接
    loop_->runTask(std::bind(&TCPServer::removeConnectionInLoop, this, conn));
}

void TCPServer::removeConnectionInLoop(const TCPConnectionPtr& conn)
{
    LOG_DEBUG << "TCPServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
    auto it = std::find(connections_.begin(), connections_.end(), conn);
    if (it != connections_.end()) {
        LOG_TRACE << "[8]TCPConnection use count: " << conn.use_count();
        connections_.erase(it);
        LOG_TRACE << "[9]TCPConnection use count: " << conn.use_count();
    }
    EventLoop* loop = conn->getLoop();
    // 确保事件执行完毕后执行销毁
    loop->pushTask(std::bind(&TCPConnection::connectDestroyed, conn));
    LOG_TRACE << "[10]TCPConnection use count: " << conn.use_count();
}

void TCPServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}