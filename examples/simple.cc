#include "TCPServer.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Logger.h"

#include <ctime>
#include <sys/time.h>
#include <cstdio>
#include <thread>

#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

class EchoServer {
public:
    EchoServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop, "EchoServer") {
        server.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start() {
        //server.setThreadNum(4);
        server.start();
    }
private:
    TCPServer server;
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer) {
        LOG_INFO << "echo " << buffer.size() << " bytes";
        conn->send(std::move(buffer));
    }
};

class DiscardServer {
public:
    DiscardServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop, "DiscardServer") {
        server.setMessageCallback(std::bind(&DiscardServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start() {
        //server.setThreadNum(4);
        server.start();
    }
private:
    TCPServer server;
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer)
    {
        std::string discard;
        discard.swap(buffer);
        LOG_INFO << "discards " << discard.size() << " bytes";
    }
};

class DaytimeServer {
public:
    DaytimeServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop, "DaytimeServer") {
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
    TimeServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop, "TimeServer") {
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
    ChargenServer(std::string host, uint16_t port, EventLoop *loop)
        : server_(host, port, loop, "ChargenServer"),
        startTime_(clock()),
        messageCount_(0)
    {
        createMessage();
        server_.setConnectionCallback(
            std::bind(&ChargenServer::onConnection, this, std::placeholders::_1));
        server_.setWriteCompleteCallback(
            std::bind(&ChargenServer::onWriteComplete, this, std::placeholders::_1));
        loop->runEvery(3.0, std::bind(&ChargenServer::printThroughput, this));
        Logger::setLogLevel(Logger::DEBUG);
    }
    void start() {
        //server.setThreadNum(4);
        server_.start();
    }
private:
    TCPServer server_;
    std::string message_;
    clock_t startTime_;
    long messageCount_;

    void createMessage() 
    {
        std::string line;
        for (int i = 33; i < 127; ++i)
        {
        line.push_back(char(i));
        }
        line += line;

        for (size_t i = 0; i < 127-33; ++i)
        {
        message_ += line.substr(i, 72) + '\n';
        }
    }

    void onConnection(const TCPConnectionPtr& conn) 
    {
        if (conn->connected()) {
            conn->send(message_);
        }
    }

    void onWriteComplete(const TCPConnectionPtr& conn) 
    {
        messageCount_ += message_.size();
        if (conn->connected()) {
            conn->send(message_);
        }
    }

    void printThroughput()
    {
        clock_t endTime = clock();
        double interval = static_cast<double>(endTime - startTime_) / CLOCKS_PER_SEC;
        printf("%4.3f MiB/s\n", static_cast<double>(messageCount_)/interval/1024/1024);
        messageCount_ = 0;
        startTime_ = endTime;
    }
};

int main(int argc, char *argv[])
{
    EventLoop eventLoop;

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