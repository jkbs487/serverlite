#include "ThreadPool.h"
#include "Logger.h"

#include <thread>
#include <stdio.h>
#include <unistd.h>  // usleep

using namespace slite;

void print()
{
    LOG_INFO << "tid=" << std::this_thread::get_id();
}

void printString(const std::string& str)
{
    printf("%s\n", str.c_str());
    usleep(100*1000);
}

void test(int maxSize)
{
    LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
    ThreadPool pool("MainThreadPool");
    pool.setMaxQueueSize(maxSize);
    pool.start(5);

    LOG_WARN << "Adding";
    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(std::bind(printString, std::string(buf)));
    }
    LOG_WARN << "Done";

    //muduo::CountDownLatch latch(1);
    //pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));
    //latch.wait();
    pool.stop();
}

/*
 * Wish we could do this in the future.
void testMove()
{
  muduo::ThreadPool pool;
  pool.start(2);

  std::unique_ptr<int> x(new int(42));
  pool.run([y = std::move(x)]{ printf("%d: %d\n", muduo::CurrentThread::tid(), *y); });
  pool.stop();
}
*/

void longTask(int num)
{
    printf("longTask %d\n", num);
    std::this_thread::sleep_for(std::chrono::microseconds(3000000));
}

void test2()
{
    LOG_WARN << "Test ThreadPool by stoping early.";
    ThreadPool pool("ThreadPool");
    pool.setMaxQueueSize(5);
    pool.start(3);

    std::thread thread1([&pool]()
    {
        for (int i = 0; i < 20; ++i)
        {
        pool.run(std::bind(longTask, i));
        }
    });

    std::this_thread::sleep_for(std::chrono::microseconds(5000000));
    LOG_WARN << "stop pool";
    pool.stop();  // early stop

    thread1.join();
    // run() after stop()
    pool.run(print);
    LOG_WARN << "test2 Done";
}

int main()
{
    test(0);
    test(1);
    test(5);
    test(10);
    test(50);
    test2();
}
