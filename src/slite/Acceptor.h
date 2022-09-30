#pragma once

#include <functional>
#include <memory>

namespace slite
{

class EventLoop;
class Channel;
class TCPHandle;

class Acceptor
{
public:
    using NewConnectionCallback = std::function<void (std::shared_ptr<TCPHandle> handle)>;

    Acceptor(std::string host, uint16_t port, EventLoop *loop);
    ~Acceptor();

    void listen();
    bool listening() const { return listening_; }

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
private:
    void handleRecv();
    
    EventLoop *loop_;
    bool listening_;
    std::shared_ptr<TCPHandle> serverHandle_;
    std::unique_ptr<Channel> acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
};

} // namespace tcpserver