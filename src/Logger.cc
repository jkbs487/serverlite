#include "Logger.h"

#include <ctime>
#include <thread>
#include <fstream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

using namespace tcpserver;

const char* logLevelName[6] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};

void Logging::append(const std::string& logLine) 
{   
    size_t written = 0;

    while (written != logLine.size())
    {
        size_t remain = logLine.size() - written;
        size_t n = ::fwrite_unlocked(logLine.c_str(), 1, logLine.size(), fp_);
        if (n != remain)
        {
        int err = ferror(fp_);
        if (err)
        {
            fprintf(stderr, "Logging::append() failed %s\n", strerror(err));
            break;
        }
        }
        written += n;
    }

    //writtenBytes_ += written;

}

std::string Logging::generateFileName(std::string baseName)
{
    std::string fileName;
    fileName.reserve(baseName.size() + 64);
    fileName = baseName;

    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    gmtime_r(&now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    fileName += timebuf;

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, "%d", ::getpid());
    fileName += pidbuf;

    fileName += ".log";

    return fileName;
}

void Logging::flush()
{
    ::fflush(fp_);
}

static Logging g_logging;

void defaultOutput(const std::string& output)
{
    g_logging.append(output);
}

static Logger::LogLevel g_level;
static OutputFunc g_output = defaultOutput;

Logger::LogLevel Logger::logLevel()
{
    return g_level;
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
    sstream_ << std::endl;
    g_output(sstream_.str());
    if (level_ == FATAL) {
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