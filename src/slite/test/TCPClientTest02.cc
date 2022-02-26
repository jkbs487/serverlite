// TcpClient destructs when TcpConnection is connected but unique.
#include "slite/EventLoop.h"
#include "slite/TCPClient.h"
#include "slite/Logger.h"

using namespace slite;

void threadFunc(EventLoop* loop)
{
    TCPClient client("127.0.0.1", 1234, loop, "TcpClient");
    client.connect();

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    // client destructs when connected.
}

int main(int argc, char* argv[])
{
    Logger::setLogLevel(Logger::DEBUG);

    EventLoop loop;
    loop.runAfter(3.0, std::bind(&EventLoop::quit, &loop));
    std::thread thr(std::bind(threadFunc, &loop));
    loop.loop();
    thr.join();
}
