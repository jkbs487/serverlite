#include "Acceptor.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

Acceptor::Acceptor(std::string host, uint16_t port, EventLoop *loop):
    loop_(loop), 
    acceptFd_(socket(AF_INET, SOCK_STREAM, 0)), 
    acceptChannel_(new Channel(loop, acceptFd_)),
    nullFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(nullFd_ >= 0);
    
    int one = 1;
    setsockopt(acceptFd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    int flags = fcntl(acceptFd_, F_GETFL, 0); 
    fcntl(acceptFd_, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(acceptFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        LOG_FATAL << "bind error " << strerror(errno);
    }

    acceptChannel_->setRecvCallback(std::bind(&Acceptor::handleRecv, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_->disableAll();
    acceptChannel_->remove();
    delete acceptChannel_;
    ::close(nullFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    ::listen(acceptFd_, SOMAXCONN);
    listening_ = true;
    acceptChannel_->enableRecv();
}

void Acceptor::handleRecv()
{
    struct sockaddr_in peerAddr;
    ::bzero(&peerAddr, 0);
    socklen_t peerLen = sizeof(peerAddr);

    int connfd = ::accept(acceptChannel_->fd(), reinterpret_cast<struct sockaddr*>(&peerAddr), &peerLen);
    if (connfd < 0) {
        LOG_ERROR << "accept error: " << strerror(errno);

        // 防止因为 fd 数量上限导致 accept 收到的 connfd 无法被 close，致使
        // 事件循环器一直有读事件 
        if (errno == EMFILE) {
            ::close(nullFd_);
            nullFd_ = ::accept(acceptFd_, NULL, NULL);
            ::close(nullFd_);
            nullFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
        return;
    }
    if (newConnectionCallback_)
        newConnectionCallback_(connfd);
    else {
        ::close(connfd);
    } 
}