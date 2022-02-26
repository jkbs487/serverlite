#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "Logging.h"

namespace slite
{

class AsyncLogger
{
public:
    AsyncLogger(const std::string& baseName, size_t roolSize, int flushInterval = 3);
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
    size_t roolSize_;
    int flushInterval_;
    int bufferSize_;
    std::string baseName_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    std::unique_ptr<std::string> buffer_;
    std::unique_ptr<std::string> nextBuffer_;
    std::vector<std::unique_ptr<std::string>> buffers_;

    static const int kBufferSize = 686000;
};

} // namespace tcpserver