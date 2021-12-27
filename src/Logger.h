#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <functional>

typedef std::function<void(std::string)> OutputFunc;

class Logging
{
public:
    Logging()
    : fp_(::fopen("/dev/fd/1", "w"))
    {
        if (!fp_) {
            perror("fopen");
            abort();
        }
    }

    Logging(std::string baseName) 
    : fp_(::fopen(generateFileName(baseName).c_str(), "w"))
    {
        if (!fp_) {
            perror("fopen");
            abort();
        }
    }

    ~Logging()
    {
        if (fp_)
            ::fclose(fp_);
    }

    void append(const std::string& logLine);
private:
    std::string generateFileName(std::string baseName);
    FILE* fp_;
};

class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    Logger(std::string file, int line, LogLevel level);
    Logger(std::string file, std::string func, int line, LogLevel level);
    Logger(const Logger&) = delete;
    Logger& operator =(const Logger&) = delete;
    ~Logger();

    std::stringstream& stream() { return sstream_; }
    static LogLevel logLevel();
    static void setOutput(OutputFunc output);
    static void setLogLevel(LogLevel level);
private:
    std::string generateFileName(std::string baseName);
    std::string getFormatTime();

    LogLevel level_;
    std::stringstream sstream_;
};

#define LOG_TRACE if (Logger::logLevel() <= Logger::TRACE) \
    Logger(__FILE__, __func__, __LINE__, Logger::TRACE).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
    Logger(__FILE__,__func__, __LINE__, Logger::DEBUG).stream()
#define LOG_INFO if (Logger::logLevel() <= Logger::INFO) \
    Logger(__FILE__, __LINE__, Logger::INFO).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()