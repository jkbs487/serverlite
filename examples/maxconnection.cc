#include "TCPServer.h"
#include "EventLoop.h"

#include <iostream>
#include <unistd.h>

class EchoServer
{
public:
    EchoServer(std::string host, uint16_t port, int maxConn);
    void start();

private:
    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer);

    TCPServer server_;
    EventLoop loop_;
    int numConnected_;
    const int kMaxConnections_;
};

EchoServer::EchoServer(std::string host, uint16_t port, int maxConn):
    server_(host, port, &loop_, "EchoServer"), numConnected_(0), kMaxConnections_(maxConn)
{
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
}

void EchoServer::start()
{
    server_.start();
    loop_.loop();
}

void EchoServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected())
    {
        ++numConnected_;
        if (numConnected_ > kMaxConnections_)
        {
        conn->shutdown();
        //conn->forceCloseWithDelay(3.0);  // > round trip of the whole Internet.
        }
    }
    else
    {
        --numConnected_;
    }
    std::cout << "numConnected = " << numConnected_ << std::endl;
}

void EchoServer::onMessage(const TCPConnectionPtr& conn, std::string& buffer)
{
    std::string msg;
    msg.swap(buffer);
    std::cout << conn->name() << " echo " << msg.size() << std::endl;
    conn->send(msg);
}

int main(int argc, char *argv[])
{
    std::cout << "pid = " << getpid() << std::endl;

    int maxConnections = 5;
    if (argc > 1)
    {
        maxConnections = atoi(argv[1]);
    }
    std::cout << "maxConnections = " << maxConnections << std::endl;
    EchoServer server("0.0.0.0", 10001, maxConnections);
    server.start();
}