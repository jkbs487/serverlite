#include "Logger.cc"

int main()
{
    Logger::setLogLevel(Logger::TRACE);
    LOG_INFO << "test";
    LOG_DEBUG << "test";
    LOG_TRACE << "test";
    LOG_ERROR << "test";
    LOG_WARN << "test";
    LOG_FATAL << "test";
}