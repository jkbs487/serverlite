#include "Logger.h"
#include "Logging.h"

#include <unistd.h>
#include <memory>
#include <cstring>

using namespace slite;

std::unique_ptr<Logging> g_logging;

void outputFunc(std::string msg)
{
    g_logging->append(msg);
}

void flushFunc()
{
    g_logging->flush();
}

int main(int argc, char* argv[])
{
  char name[256] = { '\0' };
  strncpy(name, argv[0], sizeof name - 1);
  g_logging.reset(new Logging(::basename(name), 200*1000));
  Logger::setOutput(outputFunc);
  Logger::setFlush(flushFunc);

  std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;

    usleep(1000);
  }
}
