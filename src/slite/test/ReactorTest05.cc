#include "slite/EventLoop.h"
#include "slite/Logger.h"

using namespace slite;

#include <unistd.h>

int g_flag = 0;
EventLoop* g_loop;

void run4()
{
    LOG_INFO << "run4(): pid = " << getpid() << ", flag = " << g_flag;
    g_loop->quit();
}

void run3()
{
    LOG_INFO << "run3(): pid = " << getpid() << ", flag = " << g_flag;
    g_loop->runAfter(2, run4);
    g_flag = 3;
}

void run2()
{
    LOG_INFO << "run2(): pid = " << getpid() << ", flag = " << g_flag;
    g_loop->pushTask(run3);
}

void run1()
{
    g_flag = 1;
    LOG_INFO << "run1(): pid = " << getpid() << ", flag = " << g_flag;
    g_loop->runTask(run2);
    g_flag = 2;
}

int main()
{
    LOG_INFO << "main(): pid = " << getpid() << ", flag = " << g_flag;

    EventLoop loop;
    g_loop = &loop;

    loop.runAfter(2, run1);
    loop.loop();

    LOG_INFO << "main(): pid = " << getpid() << ", flag = " << g_flag;
}