#include "TCPHandle.h"
#include "Logger.h"

#include <sys/types.h> // bind setsockopt
#include <sys/socket.h> // bind setsockopt
#include <unistd.h>
#include <fcntl.h>  // fnctl
#include <arpa/inet.h> // htons
#include <string.h> // memset
#include <netinet/tcp.h> // TCP_NODELAY

using namespace slite;

TCPHandle::TCPHandle() {
    fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (fd_ == -1) {
        LOG_FATAL << "socket error: " << strerror(errno);
    }
    memset(&peerAddr_, 0, sizeof peerAddr_);
    memset(&localAddr_, 0, sizeof localAddr_);
    LOG_TRACE << "fd " << fd_ << " open";
}

TCPHandle::TCPHandle(int fd, const SockAddr& peerAddr, const SockAddr& localAddr)
    : fd_(fd), 
    peerAddr_(peerAddr), 
    localAddr_(localAddr)
{
}

TCPHandle::~TCPHandle() {
    ::close(fd_);
    if (nullFd_ > 0) ::close(nullFd_);
    LOG_TRACE << "fd " << fd_ << " close";
}

void TCPHandle::bind(uint16_t port, const std::string& ipAddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipAddr.c_str());

    if (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        LOG_FATAL << "bind error: " << strerror(errno);
    }
}

void TCPHandle::listen()
{
    int ret = ::listen(fd_, SOMAXCONN);
    if (ret == -1) {
        LOG_FATAL << "listen error: " << strerror(errno);
    }
    nullFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
}

bool TCPHandle::accept(std::shared_ptr<TCPHandle>& handle)
{
    socklen_t peerLen = sizeof peerAddr_;
    int connfd = ::accept(fd_, reinterpret_cast<struct sockaddr*>(&peerAddr_), &peerLen);
    if (connfd < 0)
    {
        int savedErrno = errno;
        LOG_ERROR << "accept error: " << strerror(errno);
        switch (savedErrno)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
        {
            ::close(nullFd_);
            nullFd_ = ::accept(fd_, NULL, NULL);
            ::close(nullFd_);
            nullFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            return false;
        }
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            LOG_FATAL << "unexpected error of accept: " << strerror(errno);
            break;
        default:
            LOG_FATAL << "unknown error of accept: " << strerror(errno);
            break;
        }
    }
    socklen_t localLen = sizeof localAddr_;
    ::getsockname(connfd, reinterpret_cast<struct sockaddr*>(&localAddr_), &localLen);
    handle = std::make_shared<TCPHandle>(connfd, peerAddr_, localAddr_);
    return true;
}

bool TCPHandle::connect(uint16_t port, const std::string& ipAddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipAddr.c_str());

    int ret = ::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            return true;
        
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            return false;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_FATAL << "connect error:" << strerror(errno);
            break;
        
        default:
            LOG_FATAL << "upexpected connect error:" << strerror(errno);
            break;
    }
    socklen_t localLen = sizeof localAddr_;
    ::getsockname(fd_, reinterpret_cast<struct sockaddr*>(&localAddr_), &localLen);
    return true;
}

void TCPHandle::shutdown()
{
    ::shutdown(fd_, SHUT_WR);
}

ssize_t TCPHandle::send(const std::string& data)
{
    ssize_t ret = ::send(fd_, data.c_str(), data.size(), 0);
    if (ret == -1 && errno != EWOULDBLOCK)
         LOG_ERROR << "send error: " << strerror(errno);
    return ret;
}

ssize_t TCPHandle::recv(std::string& data)
{
    char buffer[65535];
    memset(buffer, 0, sizeof buffer);
    ssize_t ret = ::recv(fd_, buffer, sizeof buffer, 0);
    if (ret == -1)
        LOG_ERROR << "recv error: " << strerror(errno);
    else
        data = std::string(buffer, ret);
    return ret;
}

void TCPHandle::setReuseAddr()
{
    int one = 1;
    int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (ret == -1) {
        LOG_ERROR << "setsockopt - SO_REUSEADDR failed: " << strerror(errno);
    }
}

void TCPHandle::setNonBlock()
{
    int flags = fcntl(fd_, F_GETFL, 0); 
    int ret = fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        LOG_ERROR << "fcntl O_NONBLOCK failed: " << strerror(errno);
    }
}

void TCPHandle::setTCPNoDelay()
{
    int optval = 1;
    int ret = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret == -1) {
        LOG_ERROR << "setsockopt TCP_NODELAY failed: " << strerror(errno);
    }
}

void TCPHandle::closeTCPNoDelay()
{
    int optval = 0;
    int ret = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret == -1) {
        LOG_ERROR << "setsockopt TCP_NODELAY failed: " << strerror(errno);
    }
}

bool TCPHandle::getSocketError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        char errnoBuf[512];
        LOG_WARN << "getsockopt - SO_ERROR = " << strerror_r(errno, errnoBuf, sizeof errnoBuf);
        return false;
    } else {
        char buffer[512];
        ::bzero(buffer, sizeof buffer);
        LOG_ERROR << "handleError: " << strerror_r(optval, buffer, sizeof buffer);
    }
    return true;
}

std::string TCPHandle::getTCPInfo()
{
    char buf[1024];
    struct tcp_info tcpi;
    socklen_t len = sizeof(tcpi);
    ::bzero(&tcpi, len);
    ::getsockopt(fd_, SOL_TCP, TCP_INFO, &tcpi, &len);

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

std::string TCPHandle::localIp()
{
    return std::string(inet_ntoa(localAddr_.sin_addr));
}

uint16_t TCPHandle::localPort()
{
    return localAddr_.sin_port;
}

std::string TCPHandle::peerIp()
{
    return std::string(inet_ntoa(peerAddr_.sin_addr));
}

uint16_t TCPHandle::peerPort()
{
    return peerAddr_.sin_port;
}