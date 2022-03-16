#include "TCPConnection.h"

#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <assert.h>

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

TCPConnection::TCPConnection(EventLoop *loop, int fd, struct sockaddr_in localAddr, struct sockaddr_in peerAddr, std::string name):
    context_(nullptr),
    name_(name),
    loop_(loop), 
    sockfd_(fd), 
    localAddr_(std::string(inet_ntoa(localAddr.sin_addr))),
    localPort_(localAddr.sin_port),
    peerAddr_(std::string(inet_ntoa(peerAddr.sin_addr))),
    peerPort_(peerAddr.sin_port),
    state_(ConnState::CONNECTING), 
    channel_(new Channel(loop, fd))
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
    ::close(sockfd_);
}

std::string TCPConnection::getTcpInfoString() const
{
    char buf[1024];
    struct tcp_info tcpi;
    socklen_t len = sizeof(tcpi);
    ::bzero(&tcpi, len);
    ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, &tcpi, &len);

    snprintf(buf, len, "unrecovered=%u "
            "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
            "lost=%u retrans=%u rtt=%u rttvar=%u "
            "sshthresh=%u cwnd=%u total_retrans=%u",
            tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
            tcpi.tcpi_rto,          // Retransmit timeout in usec
            tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
            tcpi.tcpi_snd_mss,
            tcpi.tcpi_rcv_mss,
            tcpi.tcpi_lost,         // Lost packets
            tcpi.tcpi_retrans,      // Retransmitted packets out
            tcpi.tcpi_rtt,          // Smoothed round trip time in usec
            tcpi.tcpi_rttvar,       // Medium deviation
            tcpi.tcpi_snd_ssthresh,
            tcpi.tcpi_snd_cwnd,
            tcpi.tcpi_total_retrans);  // Total retransmits for entire connection

    return std::string(buf);
}

void TCPConnection::openTCPNoDelay()
{
    int optval = 1;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
}

void TCPConnection::closeTCPNoDelay()
{
    int optval = 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
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
        nsend = ::send(channel_->fd(), data.c_str(), data.size(), 0);
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
                LOG_ERROR << "send error: " << strerror(errno);
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

// block!
void TCPConnection::sendFile(std::string filePath)
{
    const int count = 1024 * 1024;
    int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd < 0) {
        LOG_ERROR << "open error: " << strerror(errno);
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
}

void TCPConnection::handleRecv(int64_t receiveTime)
{
    loop_->assertInLoopThread();
    int fd = channel_->fd();
    char buffer[65535];
    memset(buffer, 0, sizeof buffer);
    assert(channel_->isRecving());
    
    ssize_t ret = ::recv(fd, buffer, sizeof buffer, 0);
    recvBuf_.append(buffer, ret);
    if (ret < 0 && errno != EINTR && errno != EWOULDBLOCK) {
        LOG_ERROR << "recv error: " << strerror(errno);
        handleError();
    } else if (ret == 0) {
        handleClose();
    } else {
        messageCallback_(shared_from_this(), recvBuf_, receiveTime);
    }
    return;
}

void TCPConnection::handleSend()
{
    loop_->assertInLoopThread();
    if (channel_->isSending()) {
        ssize_t nsend = ::send(channel_->fd(), sendBuf_.data(), sendBuf_.size(), 0);
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
        ::shutdown(channel_->fd(), SHUT_WR);
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
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        perror("getsockopt");
    } else {
        char buffer[512];
        ::bzero(buffer, sizeof buffer);
        LOG_ERROR << "handleError: " << strerror_r(optval, buffer, sizeof buffer);
    }
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