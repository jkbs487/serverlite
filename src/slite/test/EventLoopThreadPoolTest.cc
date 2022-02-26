#include "slite/EventLoopThreadPool.h"
#include "slite/EventLoop.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>

using namespace slite;

void print(EventLoop* p = NULL)
{
    std::cout << "main(): print: pid = " << getpid() << ", tid = " << std::this_thread::get_id() << ", loop = " << p << std::endl;
}

void init(EventLoop* p)
{
    std::cout << "init(): print: pid = " << getpid() << ", tid = " << std::this_thread::get_id() << ", loop = " << p << std::endl;
}

int main()
{
  print();

  EventLoop loop;
  //loop.runAfter(11, std::bind(&EventLoop::quit, &loop));

  {
    printf("Single thread %p:\n", &loop);
    EventLoopThreadPool model(&loop);
    model.setThreadNum(0);
    model.start();
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
  }

  {
    printf("Another thread:\n");
    EventLoopThreadPool model(&loop);
    model.setThreadNum(1);
    model.start();
    EventLoop* nextLoop = model.getNextLoop();
    nextLoop->runAfter(2, std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop == model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
    ::sleep(3);
  }

  {
    printf("Three threads:\n");
    EventLoopThreadPool model(&loop);
    model.setThreadNum(3);
    model.start();
    EventLoop* nextLoop = model.getNextLoop();
    nextLoop->runTask(std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
  }

  loop.loop();
}

