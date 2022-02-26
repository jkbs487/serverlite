#pragma once

#include "TCPConnection.h"

#include <functional>
#include <memory>
#include <mutex>

namespace slite
{

typedef std::function<void (const TCPConnectionPtr& conn)> ConnectionCallback;
typedef std::function<void (const TCPConnectionPtr& conn, std::string&, int64_t)> MessageCallback;
typedef std::function<void (const TCPConnectionPtr& conn)> WriteCompleteCallback;

class EventLoop;
class Connector;

class TCPClient
{
public:
    TCPClient(std::string host, uint16_t port, EventLoop *loop, std::string name);
    ~TCPClient();
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = std::move(cb);
    }
    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = std::move(cb);
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = std::move(cb);
    }
    void connect();
    void disconnect();
    void stop();

private:
    void newConnection(int sockfd);
    void removeConnection(const TCPConnectionPtr& conn);
   
    EventLoop* loop_;
    std::string host_;
    uint16_t port_;
    std::string name_;
    std::mutex mutex_;
    std::shared_ptr<Connector> connector_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    TCPConnectionPtr connection_;
    int nextConnId_;
};

} // namespace tcpserver