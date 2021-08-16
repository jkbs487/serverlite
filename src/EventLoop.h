#pragma once

#include <vector>

class Channel;

struct epoll_event;

class EventLoop
{
public:
    explicit EventLoop();
    ~EventLoop();

    void loop();

    void createChannel(Channel *channel);
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

private:
    int epfd_;
    bool looping_;
    std::vector<struct epoll_event> events_;
};