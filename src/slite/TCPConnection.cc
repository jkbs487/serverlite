#include "TCPConnection.h"

#include "Channel.h"
#include "Logger.h"
#include "TCPHandle.h"
#include "EventLoop.h"
#include <assert.h>
#include <unistd.h>

using namespace slite;

void slite::defaultConnectionCallback(const TCPConnectionPtr& conn)
{
    LOG_INFO << conn->localAddr() << ":" << conn->localPort()
    << " -> " << conn->peerAddr() << ":" << conn->peerPort()
    << (conn->connected() ? " UP" : " DOWN");
}

void slite::defaultMessageCallback(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    std::string data;
    data.swap(buffer);
    LOG_INFO << "recv: " << data.c_str() << ", bytes: " << data.size();
}

TCPConnection::TCPConnection(EventLoop *loop, std::shared_ptr<TCPHandle> handle, std::string name):
    context_(nullptr),
    name_(name),
    loop_(loop), 
    handle_(handle),
    state_(ConnState::CONNECTING), 
    channel_(new Channel(loop, handle_->fd()))
{
    // register event callback to eventloop
    channel_->setRecvCallback(std::bind(&TCPConnection::handleRecv, this, std::placeholders::_1));
    channel_->setSendCallback(std::bind(&TCPConnection::handleSend, this));
    channel_->setCloseCallback(std::bind(&TCPConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TCPConnection::handleError, this));
    
    LOG_DEBUG << "ctor [" << name_ << "] at fd=" 
    << channel_->fd() << " state=" << stateToString();
}

TCPConnection::~TCPConnection() 
{
    LOG_DEBUG << "dtor [" << name_ << "] at fd=" 
    << channel_->fd() << " state=" << stateToString();
}

std::string TCPConnection::peerAddr() 
{ 
    return handle_->peerIp(); 
}

uint16_t TCPConnection::peerPort()
{ 
    return handle_->peerPort(); 
}

std::string TCPConnection::localAddr() 
{ 
    return handle_->localIp(); 
}

uint16_t TCPConnection::localPort()
{ 
    return handle_->localPort(); 
}

std::string TCPConnection::getTcpInfoString() const
{
    return handle_->getTCPInfo();
}

void TCPConnection::openTCPNoDelay()
{
    handle_->setTCPNoDelay();
}

void TCPConnection::closeTCPNoDelay()
{
    handle_->closeTCPNoDelay();
}

void TCPConnection::send(const std::string& data)
{
    if (loop_->isInLoopThread()) {
        sendInLoop(data);
    } else {
        loop_->runTask(std::bind(&TCPConnection::sendInLoop, this, data));
    }
}

void TCPConnection::sendInLoop(const std::string& data)
{
    if (state_ == DISCONNECTED) {
        LOG_INFO << "disconnected, give up send";
        return;
    }
    size_t remaining = data.size();
    ssize_t nsend = 0;
    bool sendError = false;

    // 先还要判断是否已经注册写事件，已注册就跳过
    // 还判断写缓存区是否为空，为空才能直接发
    if (!channel_->isSending() && sendBuf_.size() == 0) {
        nsend = handle_->send(data);
        if (nsend >= 0) {
            remaining = data.size() - nsend;
            if (remaining == 0 && writeCompleteCallback_) {
                // prevent call writecompleteCallback when data is sending
                loop_->pushTask(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {
            nsend = 0;
            // EWOULDBLOCK 表示写缓冲区满，无需处理
            if (errno != EWOULDBLOCK) {
                // SIGPIPE or RST
                if (errno == EPIPE || errno == ECONNRESET) {
                    sendError = true;
                }
            }
        }
    }

    // 如果 SIGPIPE 或者 RST，不注册任何写事件，直接退出
    // 还有剩余未发完，注册写事件
    if (!sendError && remaining > 0) {
        sendBuf_.append(data.c_str() + nsend, remaining);
        if (!channel_->isSending())
            channel_->enableSend();
    }
}

void TCPConnection::handleRecv(int64_t receiveTime)
{
    loop_->assertInLoopThread();
    assert(channel_->isRecving());
    std::string data;
    ssize_t ret = handle_->recv(data);
    if (ret < 0 && errno != EINTR && errno != EWOULDBLOCK) {
        handleError();
    } else if (ret == 0) {
        handleClose();
    } else {
        recvBuf_.append(data.c_str(), data.size());
        messageCallback_(shared_from_this(), recvBuf_, receiveTime);
    }
    return;
}

void TCPConnection::handleSend()
{
    loop_->assertInLoopThread();
    if (channel_->isSending()) {
        ssize_t nsend = handle_->send(sendBuf_);
        if (nsend > 0) {
            sendBuf_.erase(0, nsend);
            // 如果写缓存已经全部发送完毕，取消写事件
            if (sendBuf_.size() == 0) {
                channel_->disableSend();
                if (writeCompleteCallback_) {
                    loop_->pushTask(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            // do shutdown 
            if (state_ == DISCONNECTING) {
                shutdownInLoop();
            }
        } else {
            LOG_ERROR << "handle send error";
        }
    }
    else {
        LOG_INFO << "fd " << channel_->fd() << " is down, no more writing.";
    }
}

void TCPConnection::handleClose()
{
    loop_->assertInLoopThread();
    setState(DISCONNECTED);
    channel_->disableAll();
    TCPConnectionPtr guardThis(shared_from_this());
    LOG_TRACE << "[7]TCPConnection use count: " << guardThis.use_count();
    connectionCallback_(guardThis);
    // remove connection
    closeCallback_(guardThis);
    LOG_TRACE << "[11]TCPConnection use count: " << guardThis.use_count();
}

void TCPConnection::connectEstablished()
{
    setState(CONNECTED);
    LOG_TRACE << "[3]TCPConnection use count: " << shared_from_this().use_count()-1;
    channel_->enableRecv();
    channel_->tie(shared_from_this());
    connectionCallback_(shared_from_this());
    LOG_TRACE << "[4]TCPConnection use count: " << shared_from_this().use_count()-1;
}

void TCPConnection::connectDestroyed()
{
    // for tcpserver exit
    if (state_ == CONNECTED) {
        setState(DISCONNECTED);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
    LOG_TRACE << "[13]TCPConnection use count: " << shared_from_this().use_count()-1;
}

void TCPConnection::shutdown()
{
    if (state_ == CONNECTED) {
        setState(DISCONNECTING);
        loop_->runTask(std::bind(&TCPConnection::shutdownInLoop, this));
    }
}

void TCPConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    // prevent conn shutdown when data is writing 
    if (!channel_->isSending()) {
        handle_->shutdown();
    }
}

void TCPConnection::forceClose()
{
    if (state_ == CONNECTED || state_ == DISCONNECTING) {
        setState(DISCONNECTED);
        loop_->pushTask(std::bind(&TCPConnection::handleClose, this));
    }
}

void TCPConnection::handleError()
{
    handle_->getSocketError();
}

const char* TCPConnection::stateToString() const
{
  switch (state_)
  {
    case DISCONNECTED:
      return "Disconnected";
    case CONNECTING:
      return "Connecting";
    case CONNECTED:
      return "Connected";
    case DISCONNECTING:
      return "Disconnecting";
    default:
      return "unknown state";
  }
}