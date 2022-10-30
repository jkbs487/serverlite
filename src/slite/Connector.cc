#include "Connector.h"
#include "EventLoop.h"
#include "TCPHandle.h"
#include "Channel.h"
#include "Logger.h"

#include <cassert>

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
        removeChannel();
        retry();
    }
}

void Connector::connect()
{
    handle_.reset(new TCPHandle());
    handle_->setNonBlock();
    if (handle_->connect(port_, host_)) {
        connecting();
    } else {
        retry();
    }
}

void Connector::connecting()
{
    setState(CONNECTING);
    assert(!connectChannel_);
    connectChannel_.reset(new Channel(loop_, handle_->fd()));
    connectChannel_->setSendCallback(std::bind(&Connector::handleWrite, this));
    connectChannel_->setErrorCallback(std::bind(&Connector::handleError, this));
    connectChannel_->enableSend();
}

void Connector::removeChannel()
{
    connectChannel_->disableAll();
    connectChannel_->remove();
    // reset channel after network event
    loop_->pushTask(std::bind(&Connector::resetChannel, this));
}

void Connector::resetChannel()
{
    connectChannel_.reset();
}

void Connector::retry()
{
    if (connect_) {
        // client must use another new connfd to retry
        handle_.reset();
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
        removeChannel();
        if (!handle_->getSocketError()) retry();
        //else if () deal selfconn
        else {
            setState(CONNECTED);
            if (newConnectionCallback_)
                newConnectionCallback_(handle_);
        }
    } 
}

void Connector::handleError()
{
    // if new client, retry
    if (state_ == CONNECTING) {
        removeChannel();
        handle_->getSocketError();
        retry();
    }
}