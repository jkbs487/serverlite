#include "slite/EventLoop.h"
#include "slite/TCPClient.h"
#include "slite/Logger.h"

using namespace slite;

TCPClient* g_client;

void timeout()
{
  LOG_INFO << "timeout";
  g_client->stop();
}

int main(int argc, char* argv[])
{
  EventLoop loop;
  TCPClient client("127.0.0.1", 1234, &loop, "TcpClient");
  g_client = &client;
  loop.runAfter(0.0, timeout);
  loop.runAfter(1.0, std::bind(&EventLoop::quit, &loop));
  client.connect();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  loop.loop();
}
