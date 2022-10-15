#include "TCPServer.h"

#include "Logger.h"
#include "Channel.h"
#include "Acceptor.h"
#include "TCPHandle.h"
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

using namespace slite;

TCPServer::TCPServer(std::string ipAddr, uint16_t port, EventLoop *loop, const std::string& name): 
    name_(name),
    host_(ipAddr), 
    port_(port),
    loop_(loop),
    acceptor_(new Acceptor(ipAddr, port, loop)),
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
    LOG_DEBUG << "dtor [" << name_ << "]";
    for (TCPConnectionPtr& conn : connections_) {
        conn.reset();
        EventLoop* loop = conn->getLoop();
        if (loop)
            loop->runTask(std::bind(&TCPConnection::connectDestroyed, conn));
    }
}

void TCPServer::start()
{
    threadPool_->start();
    loop_->runTask(std::bind(&Acceptor::listen, acceptor_.get()));
    LOG_INFO << name_ << " listened at " << host_ << ":" << port_;
}

void TCPServer::newConnection(std::shared_ptr<TCPHandle> handle) 
{
    handle->setNonBlock();
    std::string connName;
    connName = name_ + "-" + host_ + ":" + std::to_string(port_) + "#" + std::to_string(nextConnId_++);

    LOG_DEBUG << "new connection [" << connName 
    << "] from " << handle->peerIp() << ":" << handle->peerPort();

    EventLoop* loop = threadPool_->getNextLoop();
    TCPConnectionPtr conn = std::make_shared<TCPConnection>(loop, handle, connName);

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
    LOG_DEBUG << "[" << name_ << "] - connection " << conn->name();
    auto it = std::find(connections_.begin(), connections_.end(), conn);
    if (it != connections_.end()) {
        LOG_TRACE << "[8]TCPConnection use count: " << conn.use_count();
        connections_.erase(it);
        LOG_TRACE << "[9]TCPConnection use count: " << conn.use_count();
    }
    EventLoop* loop = conn->getLoop();
    // 确保事件执行完毕后执行销毁
    if (loop) {
        loop->pushTask(std::bind(&TCPConnection::connectDestroyed, conn));
        LOG_TRACE << "[10]TCPConnection use count: " << conn.use_count();
    }
}

void TCPServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}