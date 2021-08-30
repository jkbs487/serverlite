#include "TCPServer.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <ctime>
#include <cstdio>
#include <iostream>

class EchoServer {
public:
    EchoServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop) {
        server.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start() {
        server.start();
    }
private:
    TCPServer server;
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer) {
        printf("echo %ld bytes\n", buffer.size());
        conn->send(std::move(buffer));
    }
};

class DiscardServer {
public:
    DiscardServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop) {
        server.setMessageCallback(std::bind(&DiscardServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start() {
        server.start();
    }
private:
    TCPServer server;
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer)
    {
        std::string discard;
        discard.swap(buffer);
        printf("discards %ld bytes\n", discard.size());
    }
};

class DaytimeServer {
public:
    DaytimeServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop) {
        server.setConnectionCallback(std::bind(&DaytimeServer::onConnection, this, std::placeholders::_1));
    }
    void start() {
        server.start();
    }
private:
    TCPServer server;
    void onConnection(const TCPConnectionPtr& conn)
    {
        time_t rawtime;
        struct tm *info;
        char buffer[80];

        time(&rawtime);    
        info = localtime(&rawtime);
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        if (conn->connected()) {
            conn->send(std::string(buffer));
            conn->shutdown();
        }
    }
};

class TimeServer {
public:
    TimeServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop) {
        server.setConnectionCallback(std::bind(&TimeServer::onConnection, this, std::placeholders::_1));
    }
    void start() {
        server.start();
    }
private:
    TCPServer server;
    void onConnection(const TCPConnectionPtr& conn)
    {
        time_t now = ::time(NULL);

        int32_t be32 = htobe32(static_cast<int32_t>(now));
        if (conn->connected()) {
            conn->send(std::to_string(be32));
            conn->shutdown();
        }
    }
};

class ChargenServer {
public:
    ChargenServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop) {
        createMessage();
        server.setConnectionCallback(std::bind(&ChargenServer::onConnection, this, std::placeholders::_1));
        server.setWriteCompleteCallback(std::bind(&ChargenServer::onWriteComplete, this, std::placeholders::_1));
    }
    void start() {
        server.start();
    }
private:
    TCPServer server;
    std::string message;
    void createMessage() {
        std::string line;
        for (int i = 33; i < 127; ++i)
        {
        line.push_back(char(i));
        }
        line += line;

        for (size_t i = 0; i < 127-33; ++i)
        {
        message += line.substr(i, 72) + '\n';
        }
    }
    void onConnection(const TCPConnectionPtr& conn)
    {
        if (conn->connected()) {
            conn->send(message);
        }
    }
    void onWriteComplete(const TCPConnectionPtr& conn)
    {
        if (conn->connected()) {
            conn->send(message);
        }
    }
};

int main(int argc, char *argv[])
{
    EventLoop eventLoop;
    EventLoopThread eventLoopThread;

    EchoServer echoServer("0.0.0.0", 10001, &eventLoop);
    echoServer.start();

    DiscardServer discardServer("0.0.0.0", 10002, &eventLoop);
    discardServer.start();
    
    DaytimeServer daytimeServer("0.0.0.0", 10003, &eventLoop);
    daytimeServer.start();

    TimeServer timeServer("0.0.0.0", 10004, &eventLoop);
    timeServer.start();

    ChargenServer chargenServer("0.0.0.0", 10005, &eventLoop);
    chargenServer.start();

    eventLoop.loop();

    return 0;
}