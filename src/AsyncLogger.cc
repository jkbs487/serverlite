#include "AsyncLogger.h"
#include "Logger.h"

const int kBufferSize = 686000;

AsyncLogger::AsyncLogger(const std::string& baseName, int timeoutMs)
    : running_(false), 
    timeoutMs_(timeoutMs),
    bufferSize_(kBufferSize),
    logging_(new Logging(baseName))
{
    buffer_.reserve(bufferSize_);
    nextBuffer_.reserve(bufferSize_);
}

AsyncLogger::~AsyncLogger()
{
    stop();
    delete logging_;
}


void AsyncLogger::start()
{
    if (!running_) {
        running_ = true;
        thread_ = std::thread(std::bind(&AsyncLogger::backend, this));
    }
}

void AsyncLogger::stop()
{
    running_ = false;
    cond_.notify_one();
    if (thread_.joinable())
        thread_.join();
}

void AsyncLogger::append(const std::string& log)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (bufferSize_ - buffer_.size() > log.size()) {
        buffer_.append(log);
    } else {
        buffers_.push_back(std::move(buffer_));
        if (nextBuffer_.empty()) {
            buffer_.swap(nextBuffer_);
        } else {
            std::string newBuffer;
            newBuffer.reserve(bufferSize_);
            buffer_ = newBuffer;
        }
        buffer_.append(log);
        cond_.notify_one();
    }
}

void AsyncLogger::backend()
{
    while (running_) {
        std::vector<std::string> buffersToWrite;
        std::string buffer1;
        std::string buffer2;
        buffer1.reserve(bufferSize_);
        buffer2.reserve(bufferSize_);
        buffersToWrite.reserve(16);
        std::unique_lock<std::mutex> lock(mutex_);
        {
            cond_.wait_for(lock, std::chrono::milliseconds(timeoutMs_), [this] {
                return !buffers_.empty() || !running_;
            });
            buffers_.push_back(buffer_);
            buffer_.swap(buffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_.empty()) {
                nextBuffer_.swap(buffer2);
            }
        }

        // prevent buffers too large
        if (buffersToWrite.size() > 25) {
            std::string msg = "Dropped log messages, %zd larger buffers "  + 
                std::to_string(buffersToWrite.size()-2);
            logging_->append(msg);
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        printf("buffers size=%ld\n", buffersToWrite.size());
        for (const auto &buffer: buffersToWrite) {
            //printf("buffer size=%ld\n", buffer.size());
            logging_->append(buffer);
        }

        if (!buffer1.empty()) {
            buffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            buffer1.clear();
        }

        if (!buffer2.empty()) {
            buffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            buffer2.clear();
        }

        buffersToWrite.clear();
    }
}