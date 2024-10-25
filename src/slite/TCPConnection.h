#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>

namespace slite
{

class Channel;
class TCPHandle;
class EventLoop;
class TCPConnection;

using TCPConnectionPtr = std::shared_ptr<TCPConnection>;
using ConnectionCallback = std::function<void (const TCPConnectionPtr& conn)>;
using MessageCallback = std::function<void (const TCPConnectionPtr& conn, std::string&, int64_t)>;
using CloseCallback = std::function<void (const TCPConnectionPtr& conn)>;
using WriteCompleteCallback = std::function<void (const TCPConnectionPtr& conn)>;

class TCPConnection: public std::enable_shared_from_this<TCPConnection>
{
public:
    TCPConnection(EventLoop *eventLoop, std::shared_ptr<TCPHandle> handle, std::string name);
    ~TCPConnection();
    
    void send(const std::string& data);
    //void send(const char* data, size_t len);
    void sendFile(std::string filePath);

    void setConnectionCallback(const ConnectionCallback& cb) 
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb) 
    { messageCallback_ = cb; }

    void setCloseCallback(const CloseCallback& cb) 
    { closeCallback_ = cb; }
    
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) 
    { writeCompleteCallback_ = cb; }
    
    void connectEstablished();
    void connectDestroyed();
    
    bool connected() 
    { return state_ == CONNECTED; }

    bool disConnected() 
    { return state_ == DISCONNECTED; }
    
    void shutdown();
    void forceClose();
    EventLoop* getLoop() 
    { return loop_; }

    std::string name() 
    { return name_; }

    void setContext(std::any context) 
    { context_ = context; }

    std::any getContext() 
    { return context_; }

    void removeContext() 
    { context_ = nullptr; }

    std::string getTcpInfoString() const;

    std::string peerAddr();
    uint16_t peerPort();
    std::string localAddr();
    uint16_t localPort();
    void openTCPNoDelay();
    void closeTCPNoDelay();

private:
    enum ConnState { 
        DISCONNECTED, 
        CONNECTING, 
        CONNECTED, 
        DISCONNECTING 
    };
    
    void handleRecv(int64_t receiveTime);
    void handleSend();
    void handleClose();
    void handleError();
    void setState(ConnState state) {
        state_ = state;
    }
    void sendInLoop(const std::string& data);
    void shutdownInLoop();

    const char* stateToString() const;

    std::any context_;
    std::string name_;
    EventLoop* loop_;
    std::string recvBuf_;
    std::string sendBuf_;
    std::shared_ptr<TCPHandle> handle_;
    ConnState state_;
    std::unique_ptr<Channel>channel_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

void defaultConnectionCallback(const TCPConnectionPtr& conn);
void defaultMessageCallback(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);

} // namespace tcpserver
