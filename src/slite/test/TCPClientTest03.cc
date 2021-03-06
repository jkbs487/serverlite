// TcpClient destructs in a different thread.
#include "slite/EventLoop.h"
#include "slite/EventLoopThread.h"
#include "slite/TCPClient.h"
#include "slite/Logger.h"

using namespace slite;

int main(int argc, char* argv[])
{
    Logger::setLogLevel(Logger::DEBUG);

    EventLoopThread loopThread;
    {
      TCPClient client("127.0.0.1", 1234, loopThread.startLoop(), "TcpClient");
      client.connect();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));  // wait for connect
      client.disconnect();
    }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
