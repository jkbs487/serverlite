#include <sstream>
#include <iostream>
#include <vector>
#include <functional>

namespace slite
{

using OutputFunc = std::function<void(const std::string&)>;
using FlushFunc = std::function<void()>;

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

#define LOG_TRACE if (slite::Logger::logLevel() <= slite::Logger::TRACE) \
    slite::Logger(__FILE__, __func__, __LINE__, slite::Logger::TRACE).stream()
#define LOG_DEBUG if (Logger::logLevel() <= Logger::DEBUG) \
    slite::Logger(__FILE__,__func__, __LINE__, slite::Logger::DEBUG).stream()
#define LOG_INFO if (slite::Logger::logLevel() <= slite::Logger::INFO) \
    slite::Logger(__FILE__, __LINE__, slite::Logger::INFO).stream()
#define LOG_WARN slite::Logger(__FILE__, __LINE__, slite::Logger::WARN).stream()
#define LOG_ERROR slite::Logger(__FILE__, __LINE__, slite::Logger::ERROR).stream()
#define LOG_FATAL slite::Logger(__FILE__, __LINE__, slite::Logger::FATAL).stream()

} // namespace tcpserver
