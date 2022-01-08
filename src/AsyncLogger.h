#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace tcpserver
{

class Logging;
class AsyncLogger
{
public:
    AsyncLogger(int timeoutMs);
    AsyncLogger(const std::string& baseName, int timeoutMs = 30000);
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    ~AsyncLogger();

    void start();
    void stop();
    void append(const std::string& log);

private:
    void backend();

    bool running_;
    int timeoutMs_;
    int bufferSize_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    Logging* logging_;
    std::unique_ptr<std::string> buffer_;
    std::unique_ptr<std::string> nextBuffer_;
    std::vector<std::unique_ptr<std::string>> buffers_;
};

} // namespace tcpserver