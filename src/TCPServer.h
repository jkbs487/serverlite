#pragma once

#include "TCPConnection.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

class Channel;
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

typedef std::function<void (const TCPConnectionPtr& conn)> ConnectionCallback;
typedef std::function<void (const TCPConnectionPtr& conn, std::string&)> MessageCallback;
typedef std::function<void (const TCPConnectionPtr& conn)> WriteCompleteCallback;

class TCPServer
{
public:
    TCPServer(std::string host, uint16_t port, EventLoop *eventLoop, std::string name);
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
    void setThreadNum(int numThreads);
private:
    void newConnection(int connfd);
    void removeConnection(const TCPConnectionPtr& conn);
    void removeConnectionInLoop(const TCPConnectionPtr& conn);

    std::string name_;
    std::string host_;
    uint16_t port_;
    EventLoop* loop_;
    Acceptor* acceptor_;
    //std::unordered_map<std::string, TCPConnectionPtr> connections_;
    std::vector<TCPConnectionPtr> connections_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    int nextConnId_;
};