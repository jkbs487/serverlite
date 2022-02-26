#include "TCPClient.h"

#include "EventLoop.h"
#include "Connector.h"
#include "Logger.h"

#include <cassert>

using namespace slite;

TCPClient::TCPClient(std::string host, uint16_t port, EventLoop *loop, std::string name):
    loop_(loop),
    host_(host), 
    port_(port),  
    name_(name),
    connector_(new Connector(host, port, loop)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1)
{
    connector_->setNewConnectionCallback(std::bind(&TCPClient::newConnection, this, std::placeholders::_1));
}

TCPClient::~TCPClient() 
{
    LOG_DEBUG << "TCPClient::~TCPClient [" << name_ << "]";
    bool unique = false;
    TCPConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn) {
        CloseCallback cb = std::bind(&TCPConnection::connectDestroyed, conn);
        loop_->runTask(std::bind(&TCPConnection::setCloseCallback, conn, cb));
        if (unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
        //loop_->runAfter(1, std::bind());
    }
}

void TCPClient::connect()
{
      LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << host_ << ":" << port_;
    connector_->start();
}

void TCPClient::disconnect()
{
    LOG_INFO << "TcpClient::disconnect[" << name_ << "]";
    if (connection_) {
        connection_->shutdown();
    }
}

void TCPClient::stop()
{
    connector_->stop();
}

void TCPClient::newConnection(int connfd)
{
    loop_->assertInLoopThread();
    struct sockaddr_in localAddr;
    socklen_t localLen = sizeof(localAddr);
    ::getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr), &localLen);

    struct sockaddr_in peerAddr;
    socklen_t peerLen = sizeof(peerAddr);
    ::getpeername(connfd, reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);

    std::string connName;
    connName = name_ + "-" + host_ + ":" + std::to_string(port_) + "#" + std::to_string(nextConnId_++);
    
    TCPConnectionPtr conn(new TCPConnection(loop_, connfd, localAddr, peerAddr, connName));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TCPClient::removeConnection, this, std::placeholders::_1));
    connection_ = conn;
    conn->connectEstablished();
}

void TCPClient::removeConnection(const TCPConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_.reset();
    }

    loop_->pushTask(std::bind(&TCPConnection::connectDestroyed, conn));
    connector_->restart();
}