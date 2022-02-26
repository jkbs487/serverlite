#include "Logger.h"
#include "AsyncLogger.h"
#include "ThreadPool.h"

#include <cstdio>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <sys/time.h>

using namespace slite;

int g_total;
FILE* g_file;
std::unique_ptr<Logging> logging;
AsyncLogger* g_asyncLog = nullptr;

void dummyOutput(const std::string& msg)
{
    g_total += static_cast<int>(msg.size());
    if (g_file)
    {
        ::fwrite(msg.c_str(), 1, msg.size(), g_file);
    } else if (g_asyncLog) {
        g_asyncLog->append(msg);
    } else if (logging) {
        logging->append(msg);
    }   
}

void bench(const char* type)
{
    Logger::setOutput(dummyOutput);
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t start = static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
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
    gettimeofday(&tv, NULL);
    int64_t end = static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;

    double seconds = static_cast<double>(end - start) / 1000000;
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

    ThreadPool pool("log pool");
    pool.start(4);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);

    LOG_TRACE << "Trace";
    LOG_DEBUG << "Debug";
    LOG_INFO << "Info";
    LOG_WARN << "Warn";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(Logger);

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
    g_file = nullptr;

    logging.reset(new Logging("test_log_st", 1024*1024, false));
    bench("test_log_st");

    logging.reset(new Logging("test_log_mt", 1024*1024, true));
    bench("test_log_mt");
    logging.reset();

    AsyncLogger log("asynclog", 1024*1024);
    log.start();
    g_asyncLog = &log;
    bench("test_async_log");
    log.stop();
}
