#include "TCPServer.h"
#include "EventLoop.h"

#include <utility>

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace tcpserver;

int numThreads = 0;

class EchoServer
{
    public:
    EchoServer(std::string host, uint16_t port, EventLoop* loop)
        : loop_(loop),
        server_(host, port, loop, "EchoServer")
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        server_.setThreadNum(numThreads);
    }

    void start()
    {
        server_.start();
    }
    // void stop();

    private:
    void onConnection(const TCPConnectionPtr& conn)
    {
        std::cout << inet_ntoa(conn->peerAddr().sin_addr) << " -> "
            << inet_ntoa(conn->localAddr().sin_addr) << " is "
            << (conn->connected() ? "UP" : "DOWN")  << std::endl;
        std::cout << conn->getTcpInfoString() << std::endl;

        conn->send("hello\n");
    }

    void onMessage(const TCPConnectionPtr& conn, std::string& buf)
    {
        std::string msg;
        buf.swap(msg);
        std::cout << conn->name() << " recv " << msg.size() << std::endl;
        if (msg == "exit\n")
        {
        conn->send("bye\n");
        conn->shutdown();
        }
        if (msg == "quit\n")
        {
        loop_->quit();
        }
        conn->send(msg);
    }

    EventLoop* loop_;
    TCPServer server_;
};

int main(int argc, char* argv[])
{
    std::cout << "pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;
        std::cout << "sizeof TCPConnection = " << sizeof(TCPConnection) << std::endl;
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }

    EventLoop loop;
    EchoServer server("0.0.0.0", 1234, &loop);

    server.start();

    loop.loop();
}

