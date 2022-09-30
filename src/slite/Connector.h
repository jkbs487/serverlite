#pragma once

#include <functional>
#include <memory>
#include <netinet/in.h>

namespace slite
{

class EventLoop;
class Channel;
class TCPHandle;

typedef struct sockaddr_in SockAddr; 

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void (std::shared_ptr<TCPHandle>)> NewConnectionCallback;

    Connector(std::string host, uint16_t port, EventLoop *loop);
    ~Connector();

    void start();
    void stop();
    void restart();
    void setNewConnectionCallback(const NewConnectionCallback& cb) 
    { newConnectionCallback_ = cb; }
private:
    void startInLoop();
    void stopInLoop();
    void connect();
    void retry();
    void connecting();
    void resetChannel();
    void removeChannel();
    void handleWrite();
    void handleError();
    void setState(int state) 
    { state_ = state; }

    enum States {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };
    EventLoop *loop_;
    std::shared_ptr<TCPHandle> handle_;
    std::string host_;
    uint16_t port_;
    bool connect_;
    int state_;
    int retryMs_;
    int maxRetryMs_;

    std::unique_ptr<Channel> connectChannel_;
    NewConnectionCallback newConnectionCallback_;
    SockAddr serverAddr_;
};

typedef std::shared_ptr<Connector> ConnectorPtr;

}