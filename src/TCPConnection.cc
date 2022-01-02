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

void defaultConnectionCallback(const TCPConnectionPtr& conn)
{
    LOG_INFO << inet_ntoa(conn->localAddr().sin_addr) << ":" << conn->localAddr().sin_port 
    << " -> " << inet_ntoa(conn->peerAddr().sin_addr) << ":" << conn->peerAddr().sin_port
    << (conn->connected() ? " UP" : " DOWN");
}

void defaultMessageCallback(const TCPConnectionPtr& conn, std::string& buffer)
{
    std::string data;
    data.swap(buffer);
    LOG_INFO << "recv: " << data.c_str() << ", bytes: " << data.size();
}

TCPConnection::TCPConnection(EventLoop *eventLoop, int fd, struct sockaddr_in localAddr, struct sockaddr_in peerAddr, std::string name):
    context_(nullptr),
    name_(name),
    eventLoop_(eventLoop), 
    sockfd_(fd), 
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    state_(Connecting), 
    channel_(new Channel(eventLoop, fd))
{
    // register event callback to eventloop
    channel_->setRecvCallback(std::bind(&TCPConnection::handleRecv, this));
    channel_->setSendCallback(std::bind(&TCPConnection::handleSend, this));
    channel_->setCloseCallback(std::bind(&TCPConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TCPConnection::handleError, this));
    
    LOG_DEBUG << "TCPConnection::TCPConnection [" << name_ << "] at fd=" 
    << channel_->fd() << " state=" << stateToString();
}

TCPConnection::~TCPConnection() 
{
    LOG_DEBUG << "TCPConnection::~TCPConnection [" << name_ << "] at fd=" 
    << channel_->fd() << " state=" << stateToString();
    ::close(sockfd_);
    delete channel_;
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

void TCPConnection::send(std::string data)
{
    if (eventLoop_->isInLoopThread()) {
        sendInLoop(data);
    } else {
        eventLoop_->runTask(std::bind(&TCPConnection::sendInLoop, this, data));
    }
}

void TCPConnection::sendInLoop(std::string data)
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
                // 如果runtask调用，有无限递归风险
                eventLoop_->pushTask(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {
            nsend = 0;
            perror("send");
            // EWOULDBLOCK 表示写缓冲区满，无需处理
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
}

void TCPConnection::handleRecv()
{
    eventLoop_->assertInLoopThread();
    int fd = channel_->fd();
    char buffer[65535];
    memset(buffer, 0, sizeof buffer);
    assert(channel_->isRecving());
    
    ssize_t ret = ::recv(fd, buffer, sizeof buffer, 0);
    recvBuf_.append(std::string(buffer));
    if (ret < 0 && errno != EINTR && errno != EWOULDBLOCK) {
        LOG_ERROR << "recv error: " << strerror(errno);
        handleError();
    } else if (ret == 0) {
        handleClose();
    } else {
        messageCallback_(shared_from_this(), recvBuf_);
    }
    return;
}

void TCPConnection::handleSend()
{
    eventLoop_->assertInLoopThread();
    if (state_ == Disconnected) {
        LOG_WARN << "disconnected, give up send";
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
                    eventLoop_->runTask(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            // 是否半关闭状态 
            if (state_ == Disconnecting) {
                shutdown();
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
    eventLoop_->assertInLoopThread();
    setState(Disconnected);
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
    setState(Connected);
    LOG_TRACE << "[3]TCPConnection use count: " << shared_from_this().use_count()-1;
    channel_->enableRecv();
    channel_->tie(shared_from_this());
    connectionCallback_(shared_from_this());
    LOG_TRACE << "[4]TCPConnection use count: " << shared_from_this().use_count()-1;
}

void TCPConnection::connectDestroyed()
{
    if (state_ == Connected) {
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
    LOG_TRACE << "[13]TCPConnection use count: " << shared_from_this().use_count()-1;
}

void TCPConnection::shutdown()
{
    if (state_ == Connected) {
        setState(Disconnecting);
        eventLoop_->runTask(std::bind(&TCPConnection::shutdownInLoop, this));
    }
}

void TCPConnection::shutdownInLoop()
{
    eventLoop_->assertInLoopThread();
    ::shutdown(channel_->fd(), SHUT_WR);
}

void TCPConnection::forceClose()
{
    if (state_ == Connected || state_ == Disconnecting) {
        setState(Disconnecting);
        eventLoop_->pushTask(std::bind(&TCPConnection::handleClose, this));
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
        LOG_ERROR << strerror_r(optval, buffer, sizeof buffer);
    }
}

const char* TCPConnection::stateToString() const
{
  switch (state_)
  {
    case Disconnected:
      return "Disconnected";
    case Connecting:
      return "Connecting";
    case Connected:
      return "Connected";
    case Disconnecting:
      return "Disconnecting";
    default:
      return "unknown state";
  }
}