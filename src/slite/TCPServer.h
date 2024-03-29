#pragma once

#include "TCPConnection.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

namespace slite
{

class Channel;
class Acceptor;
class TCPHandle;
class EventLoop;
class EventLoopThreadPool;

using ConnectionCallback = std::function<void (const TCPConnectionPtr& conn)>;
using MessageCallback = std::function<void (const TCPConnectionPtr& conn, std::string&, int64_t)>;
using WriteCompleteCallback = std::function<void (const TCPConnectionPtr& conn)>;

class TCPServer
{
public:
    TCPServer(std::string ipAddr, uint16_t port, EventLoop *loop, const std::string& name);
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
    
    std::string name() { return name_; }
    std::string host() { return host_; }
    uint16_t port() { return port_; }
private:
    void newConnection(std::shared_ptr<TCPHandle> handle);
    void removeConnection(const TCPConnectionPtr& conn);
    void removeConnectionInLoop(const TCPConnectionPtr& conn);

    std::string name_;
    std::string host_;
    uint16_t port_;
    EventLoop* loop_;
    std::unique_ptr<Acceptor> acceptor_;
    //std::map<std::string, TCPConnectionPtr> connections_;
    std::vector<TCPConnectionPtr> connections_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    int nextConnId_;
};

} // namespace tcpserver