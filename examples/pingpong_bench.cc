// Benchmark inspired by libevent/test/bench.c
// See also: http://libev.schmorp.de/bench.html

#include "Logger.h"
#include "EventLoop.h"
#include "Channel.h"

#include <stdio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

using namespace slite;

#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000000 + (tv1.tv_usec - tv2.tv_usec))

std::vector<int> g_pipes;
int numPipes;
int numActive;
int numWrites;
EventLoop* g_loop;
std::vector<std::unique_ptr<Channel>> g_channels;

int g_reads, g_writes, g_fired;

void readCallback(int fd, int idx)
{
    char ch;

    g_reads += static_cast<int>(::recv(fd, &ch, sizeof(ch), 0));
    if (g_writes > 0)
    {
        int widx = idx+1;
        if (widx >= numPipes)
        {
        widx -= numPipes;
        }
        ::send(g_pipes[2 * widx + 1], "m", 1, 0);
        g_writes--;
        g_fired++;
    }
    if (g_fired == g_reads)
    {
        g_loop->quit();
    }
}

std::pair<int, int> runOnce()
{
    struct timeval beforeInit;
    ::gettimeofday(&beforeInit, NULL);
    for (int i = 0; i < numPipes; ++i)
    {
        Channel& channel = *g_channels[i];
        channel.setRecvCallback(std::bind(readCallback, channel.fd(), i));
        channel.enableRecv();
    }

    int space = numPipes / numActive;
    space *= 2;
    for (int i = 0; i < numActive; ++i)
    {
        ::send(g_pipes[i * space + 1], "m", 1, 0);
    }

    g_fired = numActive;
    g_reads = 0;
    g_writes = numWrites;
    struct timeval beforeLoop;
    ::gettimeofday(&beforeLoop, NULL);

    g_loop->loop();
    
    struct timeval end;
    gettimeofday(&end, NULL);
    int iterTime = static_cast<int>(TIME_SUB_MS(end, beforeInit));
    int loopTime = static_cast<int>(TIME_SUB_MS(end, beforeLoop));
    return std::make_pair(iterTime, loopTime);
}

int main(int argc, char* argv[])
{
    numPipes = 100;
    numActive = 1;
    numWrites = 100;
    int c;
    while ((c = getopt(argc, argv, "n:a:w:")) != -1)
    {
        switch (c)
        {
        case 'n':
            numPipes = atoi(optarg);
            break;
        case 'a':
            numActive = atoi(optarg);
            break;
        case 'w':
            numWrites = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Illegal argument \"%c\"\n", c);
            return 1;
        }
    }

    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max = numPipes * 2 + 50;
    if (::setrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        perror("setrlimit");
        //return 1;  // comment out this line if under valgrind
    }
    g_pipes.resize(2 * numPipes);
    for (int i = 0; i < numPipes; ++i)
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, &g_pipes[i*2]) == -1)
        {
        perror("pipe");
        return 1;
        }
    }

    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    g_loop = &loop;

    for (int i = 0; i < numPipes; ++i)
    {
        Channel* channel = new Channel(&loop, g_pipes[i*2]);
        g_channels.emplace_back(channel);
    }

    for (int i = 0; i < 25; ++i)
    {
        std::pair<int, int> t = runOnce();
        printf("%8d %8d\n", t.first, t.second);
    }

    for (const auto& channel : g_channels)
    {
        channel->disableAll();
        channel->remove();
    }
    g_channels.clear();
}

