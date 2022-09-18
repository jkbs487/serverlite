#include <sstream>
#include <iostream>
#include <vector>
#include <functional>

namespace slite
{

typedef std::function<void(const std::string&)> OutputFunc;
typedef std::function<void()> FlushFunc;

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
    static void setFlush(FlushFunc flush);
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

} // namespace tcpserver