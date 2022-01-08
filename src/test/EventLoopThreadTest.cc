#include "EventLoopThread.h"
#include "EventLoop.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <unistd.h>

using namespace tcpserver;

void print(EventLoop* p = NULL)
{
    std::cout << "print: pid = " << getpid() << ", tid = " << std::this_thread::get_id() << ", loop = " << p << std::endl;
}

void quit(EventLoop* p)
{
    print(p);
    p->quit();
}

int main()
{
    print();

    {
        EventLoopThread thr1;  // never start
    }

    {
        // dtor calls quit()
        EventLoopThread thr2;
        EventLoop* loop = thr2.startLoop();
        loop->runTask(std::bind(print, loop));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
        // quit() before dtor
        EventLoopThread thr3;
        EventLoop* loop = thr3.startLoop();
        loop->runTask(std::bind(quit, loop));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

