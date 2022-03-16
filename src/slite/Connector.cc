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

using namespace slite;

Connector::Connector(std::string host, uint16_t port, EventLoop *loop):
    loop_(loop),
    host_(host),
    port_(port),
    connect_(false),
    state_(DISCONNECTED),
    retryMs_(1000), 
    maxRetryMs_(100000)
{
    memset(&serverAddr_, 0, sizeof serverAddr_);
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(port);
    serverAddr_.sin_addr.s_addr = inet_addr(host.c_str());
}

Connector::~Connector()
{
    LOG_DEBUG << "dtor [" << this << "]";
    // prevent Connector unexpected exit when connecting
    assert(!connectChannel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runTask(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    if (connect_) {
        connect();
    } else {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(DISCONNECTED);
    retryMs_ = 1000;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    // after event 
    connect_ = false;
    loop_->pushTask(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == CONNECTING) {
        loop_->assertInLoopThread();
        // cancel current channel for next start
        int connfd = removeChannel();
        retry(connfd);
    }
}

void Connector::connect()
{
    int connfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (connfd < 0) {
        LOG_FATAL << "socket error: " << strerror(errno);
    }
    int ret = ::connect(connfd, reinterpret_cast<struct sockaddr*>(&serverAddr_), sizeof serverAddr_);
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
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

void Connector::connecting(int connfd)
{
    setState(CONNECTING);
    assert(!connectChannel_);
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
    // reset channel after network event
    loop_->pushTask(std::bind(&Connector::resetChannel, this));
    return connfd; 
}

void Connector::resetChannel()
{
    connectChannel_.reset();
}

void Connector::retry(int connfd)
{
    if (connect_) {
        // client must use another new connfd to retry
        ::close(connfd);
        setState(DISCONNECTED);
        LOG_INFO << "Connector::retry connecting to " 
                << host_ << ":" << port_ << " in " << retryMs_ << " millseconds";
        loop_->runAfter(retryMs_/1000.0, 
                        std::bind(&Connector::startInLoop, shared_from_this()));
        retryMs_ = std::min(retryMs_ * 2, maxRetryMs_);
    }
    else {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::handleWrite()
{
    // is new client?
    if (state_ == CONNECTING) {
        int connfd = removeChannel();
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof optval);

        if (::getsockopt(connfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            char errnoBuf[512];
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << strerror_r(errno, errnoBuf, sizeof errnoBuf);
            retry(connfd);
        }
        //else if () deal selfconn
        else {
            setState(CONNECTED);
            if (newConnectionCallback_)
                newConnectionCallback_(connfd);
        }
    } 
}

void Connector::handleError()
{
    // if new client, retry
    if (state_ == CONNECTING) {
        int connfd = removeChannel();
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof optval);

        if (::getsockopt(connfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
            LOG_ERROR << "getsockopt error: ";
        } else {
            char buffer[512];
            ::bzero(buffer, sizeof buffer);
            LOG_ERROR << "handleError: " << strerror_r(optval, buffer, sizeof buffer);
        }
        retry(connfd);
    }
}