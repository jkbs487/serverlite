#include "Logger.h"

#include <ctime>
#include <thread>
#include <fstream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

using namespace slite;

const char* logLevelName[6] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

Logger::LogLevel defaultLevel()
{
    if (::getenv("SLITE_LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("SLITE_LOG_DEBUG"))
        return Logger::DEBUG;
    else 
        return Logger::INFO;
}

void defaultOutput(const std::string& output)
{
    size_t n = ::fwrite(output.c_str(), 1, output.size(), stdout);
    (void)n;
}

void defaultFlush()
{
    ::fflush(stdout);
}

Logger::LogLevel g_level = defaultLevel();
OutputFunc g_output = defaultOutput;
FlushFunc g_flush = defaultFlush;

Logger::LogLevel Logger::logLevel()
{
    return g_level;
}   

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}

void Logger::setOutput(OutputFunc output)
{
    g_output = output;
}

void Logger::setLogLevel(LogLevel level)
{
    g_level = level;
}

Logger::Logger(std::string file, int line, LogLevel level):
    level_(level)
{
    size_t pos = file.find_last_of('/');
    file = file.substr(pos+1, file.size() - pos);
    sstream_ << getFormatTime() << std::this_thread::get_id() << " ";
    sstream_ << logLevelName[level] << file << ":" << line << " -- ";
}

Logger::Logger(std::string file, std::string func, int line, LogLevel level):
    level_(level)
{
    size_t pos = file.find_last_of('/');
    file = file.substr(pos+1, file.size() - pos);
    sstream_ << getFormatTime() << std::this_thread::get_id() << " ";
    sstream_ << logLevelName[level] << file << " " << func << ":" << line << " -- ";
}

Logger::~Logger()
{
    sstream_ << "\n";
    g_output(sstream_.str());
    if (level_ == FATAL) {
        g_flush();
        abort();
    }
}

std::string Logger::getFormatTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_time;
    gmtime_r(&tv.tv_sec, &tm_time);
    char now[80];

    snprintf(now, sizeof(now), "%4d-%02d-%02d %02d:%02d:%02d ",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

    return std::string(now);
}