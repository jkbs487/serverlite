#include <queue>
#include <string>

#include "slite/Logger.h"

class MessageQueue
{
public:
    MessageQueue() {

    }

    void push(std::string data) {
        queue.push(data);
    }

    std::string pop() {
        std::string&& data = std::move(queue.back());
        queue.pop();
        return data;
    }

private:
    std::queue<std::string> queue;
};

int main() {
    MessageQueue queue;
    queue.push("hello");
    LOG_INFO << queue.pop();
}