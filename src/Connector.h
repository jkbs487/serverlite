#pragma once

#include <functional>
#include <memory>
#include <netinet/in.h>

namespace tcpserver
{

class EventLoop;
class Channel;

typedef struct sockaddr_in SockAddr; 

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void (int connfd)> NewConnectionCallback;

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
    void retry(int connfd);
    void connecting(int connfd);
    void resetChannel();
    int removeChannel();
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
    std::string host_;
    uint16_t port_;
    bool connect_;
    int state_;
    int connectFd_;
    int retryMs_;
    int maxRetryMs_;

    std::unique_ptr<Channel> connectChannel_;
    NewConnectionCallback newConnectionCallback_;
    SockAddr serverAddr_;
};

}