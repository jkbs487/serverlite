#include "AsyncLogger.h"

#include <cassert>
#include <functional>

using namespace slite;

AsyncLogger::AsyncLogger(const std::string& baseName, size_t roolSize, int flushInterval)
    : running_(false), 
    roolSize_(roolSize),
    flushInterval_(flushInterval),
    baseName_(baseName),
    buffer_(new std::string()),
    nextBuffer_(new std::string())
{
    buffer_->reserve(kBufferSize);
    nextBuffer_->reserve(kBufferSize);
}

AsyncLogger::~AsyncLogger()
{
    stop();
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
#include <iostream>
void AsyncLogger::append(const std::string& log)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (kBufferSize - buffer_->size() > log.size()) {
        buffer_->append(log);
    } else {
        buffers_.push_back(std::move(buffer_));
        if (!nextBuffer_) {
            buffer_ = std::move(nextBuffer_);
        } else {
            buffer_.reset(new std::string());
        }
        buffer_->append(log);
        cond_.notify_one();
    }
}

void AsyncLogger::backend()
{
    std::vector<std::unique_ptr<std::string>> buffersToWrite;
    std::unique_ptr<std::string> buffer1(new std::string());
    std::unique_ptr<std::string> buffer2(new std::string());
    buffer1->reserve(kBufferSize);
    buffer2->reserve(kBufferSize);
    buffersToWrite.reserve(16);
    Logging logging(baseName_, roolSize_, false);
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        {
            cond_.wait_for(lock, std::chrono::seconds(flushInterval_), [this] {
                return !buffers_.empty() || !running_;
            });
            buffers_.push_back(std::move(buffer_));
            buffer_ = std::move(buffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_) {
                nextBuffer_ = std::move(buffer2);
            }
        }

        assert(!buffersToWrite.empty());
        // prevent buffers too large
        if (buffersToWrite.size() > 25) {
            std::string msg = "Dropped log messages, %zd larger buffers "  + 
                std::to_string(buffersToWrite.size()-2);
            logging.append(msg);
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        for (const auto &buffer: buffersToWrite) {
            logging.append(*buffer);
        }

        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }

        if (!buffer1) {
            assert(!buffersToWrite.empty());
            buffer1 = std::move(buffersToWrite.back());
            buffer1->clear();
            buffersToWrite.pop_back();
        }

        if (!buffer2) {
            assert(!buffersToWrite.empty());
            buffer2 = std::move(buffersToWrite.back());
            buffer2->clear();
            buffersToWrite.pop_back();
        }

        logging.flush();
        buffersToWrite.clear();
    }

    logging.flush();
}