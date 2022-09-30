#include "Acceptor.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TCPHandle.h"
#include "Logger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

using namespace slite;

Acceptor::Acceptor(std::string host, uint16_t port, EventLoop *loop):
    loop_(loop), 
    serverHandle_(new TCPHandle()),
    acceptChannel_(new Channel(loop, serverHandle_->fd()))
{
    serverHandle_->setReuseAddr();
    serverHandle_->setNonBlock();
    serverHandle_->bind(port, host);
    acceptChannel_->setRecvCallback(std::bind(&Acceptor::handleRecv, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_->disableAll();
    acceptChannel_->remove();
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    serverHandle_->listen();
    listening_ = true;
    acceptChannel_->enableRecv();
}

void Acceptor::handleRecv()
{
    std::shared_ptr<TCPHandle> clientHandle;
    if (!serverHandle_->accept(clientHandle)) {
        return;
    }
    if (newConnectionCallback_)
        newConnectionCallback_(clientHandle);
}