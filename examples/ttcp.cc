#include "slite/TCPServer.h"
#include "slite/EventLoop.h"

#include <iostream>

using namespace slite;

struct SessionMessage {
    int32_t number;
    int32_t length;
} __attribute__ ((__packed__));

struct PayloadMessage {
    int32_t length;
    char data[0];
};

class TTCPServer
{
    TTCPServer(std::string host, uint16_t port, EventLoop *eventLoop): server(host, port, eventLoop, "TTCP") {
        server.setMessageCallback(std::bind(&TTCPServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    }

    void start() {
        server.setThreadNum(4);
        server.start();
    }
private:
    TCPServer server;
    
    void onConnection(const TCPConnectionPtr& conn) {
        if (conn->connected()) {
            
        }
        SessionMessage session = {0, 0};

    }

    void onMessage(const TCPConnectionPtr& conn, std::string& buffer)
    {
        
    }
};