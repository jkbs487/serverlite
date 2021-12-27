#include "Logger.h"
#include "AsyncLogger.h"

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

off_t kRollSize = 500*1000*1000;

AsyncLogger* g_asyncLog = NULL;

void asyncOutput(std::string log)
{
  g_asyncLog->append(log);
}

void bench(bool longLog)
{
  Logger::setOutput(asyncOutput);

  int cnt = 0;
  const int kBatch = 10000;
  std::string empty = " ";
  std::string longStr(3000, 'X');
  longStr += " ";

  for (int t = 0; t < 30; ++t)
  {
    clock_t start = clock();
    for (int i = 0; i < kBatch; ++i)
    {
      LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
               << (longLog ? longStr : empty)
               << cnt;
      ++cnt;
    }
    clock_t end = clock();
    printf("%f\n", double(end - start) / kBatch);
    //struct timespec ts = { 0, 500*1000*1000 };
    //nanosleep(&ts, NULL);
  }
}

int main(int argc, char* argv[])
{
  {
    // set max virtual memory to 2GB.
    size_t kOneGB = 1000*1024*1024;
    rlimit rl = { 2*kOneGB, 2*kOneGB };
    setrlimit(RLIMIT_AS, &rl);
  }

  printf("pid = %d\n", getpid());

  AsyncLogger log("test");
  log.start();
  g_asyncLog = &log;

  bool longLog = argc > 1;
  bench(longLog);
}