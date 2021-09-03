#include "EventLoop.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <iostream>

EventLoop* g_loop;

void callback()
{
    std::cout << "callback(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;
    EventLoop anotherLoop;
}

void threadFunc()
{
    std::cout << "callback(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;

    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    //loop.runAfter(1.0, callback);
    loop.loop();
}

int main()
{
    std::cout << "callback(): pid = " << getpid() << ", tid = " << std::this_thread::get_id() << std::endl;

    assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

    std::thread thread(threadFunc);

    loop.loop();
}
