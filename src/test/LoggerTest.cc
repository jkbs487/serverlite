#include "Logger.h"
#include "AsyncLogger.h"

#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <memory>

int g_total;
FILE* g_file;
std::unique_ptr<Logging> logging;
AsyncLogger* g_asyncLog = NULL;

void dummyOutput(const std::string& msg)
{
    g_total += static_cast<int>(msg.size());
    if (g_file)
    {
        ::fwrite(msg.c_str(), 1, msg.size(), g_file);
    } else if (g_asyncLog) {
        g_asyncLog->append(msg);
    } 
    else {
        logging->append(msg);
    }
}

void bench(const char* type)
{
    Logger::setOutput(dummyOutput);
    clock_t start = clock();
    g_total = 0;

    int n = 1000*1000;
    const bool kLongLog = false;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";
    for (int i = 0; i < n; ++i)
    {
        LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
                << (kLongLog ? longStr : empty)
                << i;
    }
    clock_t end = clock();
    double seconds = double(end - start) / CLOCKS_PER_SEC;
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
          type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
    LOG_INFO << "logInThread";
    usleep(1000);
}

int main()
{
    getppid(); // for ltrace and strace

    LOG_TRACE << "Trace";
    LOG_DEBUG << "Debug";
    LOG_INFO << "Info";
    LOG_WARN << "Warn";
    LOG_ERROR << "Error";

    logging.reset(new Logging("test"));

    sleep(1);
    bench("nop");

    char buffer[64*1024];

    g_file = fopen("/dev/null", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/dev/null");
    fclose(g_file);

    g_file = fopen("/tmp/log", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/tmp/log");
    fclose(g_file);

    AsyncLogger log("test");
    log.start();
    g_asyncLog = &log;
    bench("async");
    log.stop();
}
