#include <sys/epoll.h>
#include <string>
#include <iostream>
#include <functional>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "TCPConnection.h"

class TCPServer 
{
    typedef std::function<void (TCPConnection*)> ConnectionCallback;
    typedef std::function<void (TCPConnection*, std::string)> MessageCallback;
    typedef std::function<void (TCPConnection*)> WriteCompleteCallback;
public:
    TCPServer(std::string host, uint16_t port);
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
private:
    std::string host_;
    uint16_t port_;
    int listenfd_;
    int epfd_;
    std::vector<Channel*> channels_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    struct epoll_event events_[1024];
    void loop();
    void handleAccept();

    void removeConnection(TCPConnection *conn);
    void defaultConnectionCallback(TCPConnection *conn);
    void defaultMessageCallback(TCPConnection *conn, std::string buffer);
};

TCPServer::TCPServer(std::string host, uint16_t port): host_(host), port_(port)
{
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    signal(SIGPIPE, SIG_IGN);
    
    int one = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(listenfd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof addr) < 0) {
        perror("bind");
        exit(0);
    }
    connectionCallback_ = std::bind(&TCPServer::defaultConnectionCallback, this, std::placeholders::_1);
    messageCallback_ = std::bind(&TCPServer::defaultMessageCallback, this, std::placeholders::_1, std::placeholders::_2);
}

void TCPServer::start()
{
    listen(listenfd_, 5);

    this->epfd_ = epoll_create(1);
    struct epoll_event event;

    Channel *channel = new Channel(listenfd_);
    channel->setRecvCallback(std::bind(&TCPServer::handleAccept, this));
    event.data.ptr = channel;
    event.events = EPOLLIN;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, listenfd_, &event);
    loop();
}

void TCPServer::loop()
{
    for (;;)
    {
        int num_events = epoll_wait(epfd_, events_, 1024, -1);
        if (num_events < 0) {
            perror("epoll_wait");
            break;
        }
        if (num_events == 0) continue;

        for (int i = 0; i < num_events; i++) {
            Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
            if (events_[i].events & EPOLLIN) {
                channel->setEvent(EPOLLIN);
            }
            if (events_[i].events & EPOLLOUT) {
                channel->setEvent(EPOLLOUT);
            }
            channel->handleEvent();
        }
    }
}

void TCPServer::handleAccept() 
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof client_addr);
    socklen_t len = sizeof(client_addr);

    int clientfd = accept(listenfd_, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
    if (clientfd < 0) return;

    TCPConnection *new_conn = new TCPConnection(clientfd, epfd_);
    new_conn->setConnectionCallback(connectionCallback_);
    new_conn->setMessageCallback(messageCallback_);
    new_conn->setWriteCompleteCallback(writeCompleteCallback_);
    new_conn->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));
    new_conn->connectEstablished();
    return ;
}

void TCPServer::defaultConnectionCallback(TCPConnection *conn)
{
    if (conn->connected())
        printf("UP\n");
    else {
        printf("DOWN\n");
    }
}

void TCPServer::defaultMessageCallback(TCPConnection *conn, std::string buffer)
{
    printf("recv: %s, bytes: %ld\n", buffer.c_str(), buffer.size());
}

void TCPServer::removeConnection(TCPConnection *conn)
{
    conn->connectDestroyed();
}