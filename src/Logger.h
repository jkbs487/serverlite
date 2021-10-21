#include <fstream>
#include <iostream>
#include <vector>

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

    std::ofstream& stream() { return stream_; }
    static LogLevel logLevel();
    static void setOutput(std::string output);
    static void setLogLevel(LogLevel level);
private:
    std::ofstream stream_;
    LogLevel level_;
    std::vector<std::string> logLevelName_;
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