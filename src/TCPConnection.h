#pragma once

#include <functional>
#include <memory>
#include <netinet/in.h>

namespace tcpserver
{

class Channel;
class EventLoop;
class TCPConnection;

typedef std::shared_ptr<TCPConnection> TCPConnectionPtr;
typedef std::function<void (const TCPConnectionPtr& conn)> ConnectionCallback;
typedef std::function<void (const TCPConnectionPtr& conn, std::string&)> MessageCallback;
typedef std::function<void (const TCPConnectionPtr& conn)> CloseCallback;
typedef std::function<void (const TCPConnectionPtr& conn)> WriteCompleteCallback;

class TCPConnection: public std::enable_shared_from_this<TCPConnection>
{
public:
    TCPConnection(EventLoop *eventLoop, int fd, struct sockaddr_in localAddr, struct sockaddr_in peerAddr, std::string name);
    ~TCPConnection();
    
    void send(std::string data);
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

    struct sockaddr_in peerAddr() 
    { return peerAddr_; }

    struct sockaddr_in localAddr() 
    { return localAddr_; }

    void setContext(void* context) 
    { context_ = context; }

    void* getContext() 
    { return context_; }

    void removeContext() 
    { context_ = nullptr; }

    std::string getTcpInfoString() const;

private:
    enum ConnState { 
        DISCONNECTED, 
        CONNECTING, 
        CONNECTED, 
        DISCONNECTING 
    };
    
    void handleRecv();
    void handleSend();
    void handleClose();
    void handleError();
    void setState(ConnState state) {
        state_ = state;
    }
    void sendInLoop(std::string data);
    void shutdownInLoop();

    const char* stateToString() const;

    void* context_;
    std::string name_;
    EventLoop* loop_;
    int sockfd_;
    std::string recvBuf_;
    std::string sendBuf_;
    struct sockaddr_in localAddr_;
    struct sockaddr_in peerAddr_;
    ConnState state_;
    std::unique_ptr<Channel>channel_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

void defaultConnectionCallback(const TCPConnectionPtr& conn);
void defaultMessageCallback(const TCPConnectionPtr& conn, std::string& buffer);

} // namespace tcpserver