#include "slite/TCPClient.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"

int count;

using namespace slite;

EventLoop* g_loop;

void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    conn->send("test");
}

void onConnection(const TCPConnectionPtr& conn)
{
    if (conn->disConnected()) {
        g_loop->quit();
        return;
    }

    for (int i = 0; i < 1000; i++) {
        count++;
        conn->send("1234567890abcdefghijklmnopqrst");
    }
}

void close(const TCPConnectionPtr& conn)
{
    if (count == 10000) {
        conn->shutdown();
    }
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    g_loop = &loop;
    TCPClient discardClient("127.0.0.1", 20005, &loop, "client");
    //discardClient.setConnectionCallback(onConnection);

    discardClient.connect();
    loop.loop();
}