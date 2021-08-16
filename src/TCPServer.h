#pragma once

#include "TCPConnection.h"

#include <functional>

class Channel;
class EventLoop;

class TCPServer 
{
    typedef std::function<void (TCPConnection*)> ConnectionCallback;
    typedef std::function<void (TCPConnection*, std::string)> MessageCallback;
    typedef std::function<void (TCPConnection*)> WriteCompleteCallback;
public:
    TCPServer(std::string host, uint16_t port);
    TCPServer(std::string host, uint16_t port, EventLoop *eventLoop);
    ~TCPServer();
    void start();
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }
private:
    std::string host_;
    uint16_t port_;
    Channel *acceptChannel_;
    EventLoop *eventLoop_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    void handleAccept();

    void removeConnection(TCPConnection *conn);
    void defaultConnectionCallback(TCPConnection *conn);
    void defaultMessageCallback(TCPConnection *conn, std::string buffer);
};