#pragma once

#include <vector>
#include <unordered_map>
#include <thread>

class Channel;
struct epoll_event;

class EventLoop
{
public:
    explicit EventLoop();
    ~EventLoop();

    void loop();

    void quit();
    
    void createChannel(Channel *channel);
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

private:
    int epfd_;
    bool quit_;
    std::thread::id threadId_;
    std::vector<struct epoll_event> events_;
    std::unordered_map<int, Channel*> channels_;
};