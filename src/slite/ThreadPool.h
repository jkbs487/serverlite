// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#pragma once

#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <functional>

namespace slite
{

class ThreadPool
{
public:
    using Task = std::function<void ()>;

    explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    // Must be called before start().
    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task& cb)
    { threadInitCallback_ = cb; }

    void start(int numThreads);
    void stop();

    const std::string& name() const
    { return name_; }

    size_t queueSize() const;

    // Could block if maxQueueSize > 0
    // Call after stop() will return immediately.
    // There is no move-only version of std::function in C++ as of C++14.
    // So we don't need to overload a const& and an && versions
    // as we do in (Bounded)BlockingQueue.
    // https://stackoverflow.com/a/25408989
    void run(Task f);

 private:
    bool isFull() const;
    void runInThread();
    Task take();

    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<std::thread>> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};

class Exception : public std::exception
{
 public:
  Exception(std::string what);
  ~Exception() noexcept override = default;

  // default copy-ctor and operator= are okay.

  const char* what() const noexcept override
  {
    return message_.c_str();
  }

  const char* stackTrace() const noexcept
  {
    return stack_.c_str();
  }

 private:
  std::string message_;
  std::string stack_;
};

}  // namespace tcpserver