#include "Logger.h"

#include <ctime>
#include <thread>

Logger::LogLevel g_level;
std::string g_output = "/dev/fd/1";

Logger::LogLevel Logger::logLevel()
{
    return g_level;
}

void Logger::setOutput(std::string output)
{
    g_output = output;
}

void Logger::setLogLevel(LogLevel level)
{
    g_level = level;
}

Logger::Logger(std::string file, int line, LogLevel level):
    stream_(std::ofstream(g_output)),
    level_(level),
    logLevelName_({"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"})
{
    time_t rawtime;
    struct tm *info;
    char now[80];
    time( &rawtime );
    info = localtime( &rawtime );
    strftime(now, 80, "%Y-%m-%d %H:%M:%S", info);

    file = file.substr(file.find_last_of('/')+1, file.size()-file.find_last_of('/'));
    stream_ << now << " " << std::this_thread::get_id() << " ";
    stream_ << logLevelName_[level] << " " << file << ":" << line << " ";
}

Logger::Logger(std::string file, std::string func, int line, LogLevel level):
    stream_(std::ofstream(g_output)),
    level_(level),
    logLevelName_({"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"})
{
    time_t rawtime;
    struct tm *info;
    char now[80];
    time( &rawtime );
    info = localtime( &rawtime );
    strftime(now, 80, "%Y-%m-%d %H:%M:%S", info);
    
    file = file.substr(file.find_last_of('/')+1, file.size()-file.find_last_of('/'));
    stream_ << now << " " << std::this_thread::get_id() << " ";
    stream_ << logLevelName_[level] << " " << file << " " << func << ":" << line << " ";
}

Logger::~Logger()
{
    stream_ << std::endl;
    stream_.close();
    if (level_ == FATAL) {
        abort();
    }
}