#include "Acceptor.h"
#include "EventLoop.h"
#include "Channel.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>

Acceptor::Acceptor(std::string host, uint16_t port, EventLoop *loop):
    loop_(loop), 
    acceptFd_(socket(AF_INET, SOCK_STREAM, 0)), 
    acceptChannel_(new Channel(loop, acceptFd_))
{
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
        perror("bind");
        exit(-1);
    }

    acceptChannel_->setRecvCallback(std::bind(&Acceptor::handleRecv, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_->disableAll();
    acceptChannel_->remove();
    delete acceptChannel_;
}

void Acceptor::listen()
{
    std::cout << "Acceptor::listen" << std::endl;
    loop_->assertInLoopThread();
    ::listen(acceptFd_, 5);
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
        perror("accept");
        return;
    }
    if (newConnectionCallback_)
        newConnectionCallback_(connfd, peerAddr);
    else 
        ::close(connfd); 
}