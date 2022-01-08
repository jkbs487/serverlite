#pragma once

#include <functional>

namespace tcpserver
{

class EventLoop;
class Channel;

class Acceptor
{
public:
    typedef std::function<void (int connfd)> NewConnectionCallback;

    Acceptor(std::string host, uint16_t port, EventLoop *loop);
    ~Acceptor();

    void listen();
    bool listening() const { return listening_; }

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
private:
    void handleRecv();
    
    EventLoop *loop_;
    bool listening_;
    int acceptFd_;
    Channel *acceptChannel_;
    int nullFd_;
    NewConnectionCallback newConnectionCallback_;
};

} // namespace tcpserver