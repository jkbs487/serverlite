#include "Connector.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cassert>
#include <cerrno>

Connector::Connector(std::string host, uint16_t port, EventLoop *loop):
    loop_(loop), retryMs_(1000), maxRetryMs_(10000)
{
    memset(&serverAddr_, 0, sizeof serverAddr_);
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(port);
    serverAddr_.sin_addr.s_addr = inet_addr(host.c_str());
}

Connector::~Connector()
{
    assert(!connectChannel_);
}

void Connector::start()
{
    loop_->runTask(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    LOG_INFO << "Connector::startInLoop";
    loop_->assertInLoopThread();
    connect();
}

void Connector::stop()
{
    loop_->runTask(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    LOG_INFO << __func__;
    loop_->assertInLoopThread();
    int connfd = removeChannel();
    retry(connfd);
}

void Connector::connect()
{
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = ::connect(connfd, reinterpret_cast<struct sockaddr*>(&serverAddr_), sizeof serverAddr_);
    if (ret < 0) {
        switch (errno) {
            case 0:
            case EINPROGRESS:
            case EINTR:
            case EISCONN:
                connecting(connfd);
                break;
            
            case EAGAIN:
            case EADDRINUSE:
            case EADDRNOTAVAIL:
            case ECONNREFUSED:
            case ENETUNREACH:
                retry(connfd);
                break;

            case EACCES:
            case EPERM:
            case EAFNOSUPPORT:
            case EALREADY:
            case EBADF:
            case EFAULT:
            case ENOTSOCK:
                LOG_ERROR << "connect error" << errno;
                ::close(connfd);
                break;
            
            default:
                LOG_ERROR << "upexpected error" << errno;
                ::close(connfd);
                break;
        }
    }
}

void Connector::connecting(int connfd)
{
    connectChannel_.reset(new Channel(loop_, connfd));
    connectChannel_->setSendCallback(std::bind(&Connector::handleWrite, this));
    connectChannel_->setErrorCallback(std::bind(&Connector::handleError, this));
    connectChannel_->enableSend();
}

int Connector::removeChannel()
{
    connectChannel_->disableAll();
    connectChannel_->remove();
    int connfd = connectChannel_->fd();
    loop_->pushTask(std::bind(&Connector::resetChannel, this));
    return connfd; 
}

void Connector::resetChannel()
{
    connectChannel_.reset();
}

void Connector::retry(int connfd)
{
    ::close(connfd);
    // !!!
    loop_->runAfter(retryMs_/1000.0, std::bind(&Connector::startInLoop, this));
    retryMs_ = std::min(retryMs_ * 2, maxRetryMs_);
}

void Connector::handleWrite()
{
    int connfd = removeChannel();
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(connfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        char errnoBuf[512];
        LOG_WARN << "Connector::handleWrite - SO_ERROR = " << strerror_r(errno, errnoBuf, sizeof errnoBuf);
        retry(connfd);
    }
}

void Connector::handleError()
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(connectChannel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        perror("getsockopt");
    } else {
        char buffer[512];
        ::bzero(buffer, sizeof buffer);
        LOG_ERROR << strerror_r(optval, buffer, sizeof buffer);
    }
}