#pragma once
#include <string>
#include <memory>
#include <netinet/in.h> // struct sockaddr_in

namespace slite {

using SockAddr = struct sockaddr_in;

class TCPHandle
{
public:
    TCPHandle();
    TCPHandle(int fd, const SockAddr& peerAddr, const SockAddr& localAddr);
    ~TCPHandle();
    TCPHandle(const TCPHandle& other) = delete;
    TCPHandle& operator=(TCPHandle other) = delete;

    int fd() { return fd_; }
    void bind(uint16_t port, const std::string& addr);
    void listen();
    bool accept(std::shared_ptr<TCPHandle>& handle);
    bool connect(uint16_t port, const std::string& ipAddr);
    void shutdown();
    ssize_t send(const std::string& data);
    ssize_t recv(std::string& data);

    void setReuseAddr();
    void setNonBlock();
    void setTCPNoDelay();
    void closeTCPNoDelay();
    bool getSocketError();
    std::string getTCPInfo();
    std::string localIp();
    uint16_t localPort();
    std::string peerIp();
    uint16_t peerPort();

private:
    int fd_;
    int nullFd_;
    SockAddr peerAddr_;
    SockAddr localAddr_;
};

} // slite
