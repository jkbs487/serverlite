#include "slite/EventLoop.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <iostream>

using namespace slite;

EventLoop* g_loop;
clock_t begin;
void callback()
{
    std::cout << "callback(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;
    EventLoop anotherLoop;
}

void threadFunc()
{
    std::cout << "threadFunc(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;

    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    loop.runAfter(2.2, callback);
    loop.loop();
}   

int main()
{
    std::cout << "main(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;

    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

    std::thread thread(threadFunc);

    loop.loop();
}
