#include "slite/EventLoop.h"
#include "slite/Channel.h"

#include <sys/timerfd.h>
#include <iostream>
#include <strings.h>
#include <unistd.h>

using namespace slite;

EventLoop g_loop;

void timeout(int64_t receiveTime)
{
    std::cout << "Timeout!" << std::endl;
    g_loop.quit();
}

int main()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel* timerChannel = new Channel(&g_loop, timerfd);
    timerChannel->setRecvCallback(timeout);
    timerChannel->enableRecv();

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 2;
    howlong.it_interval.tv_sec = 2;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    g_loop.loop();
}