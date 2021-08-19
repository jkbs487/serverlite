#pragma once

#include "TCPConnection.h"

#include <functional>
#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;

typedef std::function<void (const TCPConnectionPtr& conn)> ConnectionCallback;
typedef std::function<void (const TCPConnectionPtr& conn, std::string)> MessageCallback;
typedef std::function<void (const TCPConnectionPtr& conn)> WriteCompleteCallback;

class TCPServer 
{
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
    //std::unordered_map<std::string, TCPConnectionPtr> connections_;
    std::vector<TCPConnectionPtr> connections_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    void handleAccept();

    void removeConnection(const TCPConnectionPtr& conn);
    void defaultConnectionCallback(const TCPConnectionPtr& conn);
    void defaultMessageCallback(const TCPConnectionPtr& conn, std::string buffer);
};