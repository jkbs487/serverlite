// TcpClient destructs in a different thread.
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "TCPClient.h"
#include "Logger.h"

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
