#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/EventLoopThread.h"
#include "slite/Logger.h"

#include <algorithm>

const std::string kCRLF = "\r\n";

struct Context
{
    int state;
    int count;
    size_t length;
    std::vector<std::string> cmds;
    Context(): state(0), count(0), length(0) {}
};

using ContextPtr = std::shared_ptr<Context>;

class MemoryDBServer
{
public:
    MemoryDBServer(std::string ipAddr, uint16_t port)
    : loop_(new slite::EventLoop()),
    server_(ipAddr, port, loop_, "MemoryDBServer")
{
    server_.setConnectionCallback(std::bind(&MemoryDBServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&MemoryDBServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));    
}

    ~MemoryDBServer()
    {
		if (loop_)
			delete loop_;
    }

void start()
{
    server_.start();
    loop_->loop();
}

void onMessage(const slite::TCPConnectionPtr& conn, std::string& buffer, int64_t time)
{
    ContextPtr context = std::any_cast<ContextPtr>(conn->getContext());
    if (!parse(buffer, context)) {
        LOG_ERROR << "parse error";
    }
    if (context->state == 2) {
        std::vector<std::string> cmds = context->cmds;
        std::for_each(cmds.begin(), cmds.end(), [](const std::string& s){ LOG_INFO <<"cmd = " << s; });
        if (cmds[0] == "COMMAND") {
            conn->send("*7\r\n$11\r\nsunionstore\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:1\r\n*3\r\n+@write\r\n+@set\r\n+@slow\r\n");
        } else if (cmds[0] == "GET" || cmds[0] == "get") {
            if (cmds.size() < 2) {
                conn->send("-ERR wrong number of arguments for 'get' command\r\n");
                return;
            }
            if (db_.count(cmds[1])) {
                conn->send("+" + db_[cmds[1]] + "\r\n");
            } else {
                conn->send("$-1\r\n");
            }
        } else if (cmds[0] == "SET" || cmds[0] == "set") {
            if (cmds.size() < 3) 
                conn->send("-ERR wrong number of arguments for 'set' command\r\n");
            else {
                db_[cmds[1]] = cmds[2];
                conn->send("+ok\r\n");
            }
        } else if (cmds[0] == "DEL" || cmds[0] == "del") {
            int delCount = 0;
            for (size_t i = 1; i < cmds.size(); i++) {
                if (db_.count(cmds[i])) {
                    db_.erase(cmds[i]);
                    delCount++;
                }
            }
            conn->send(":" + std::to_string(delCount) + kCRLF);
        } else {
            conn->send("-ERR unknown command "+ cmds[0] +"\r\n");
        }
        context->cmds.clear();
        context->state = 0;
    }
}

void onConnection(const slite::TCPConnectionPtr& conn)
{
   if (conn->connected()) {
        ContextPtr context = std::make_shared<Context>();
        conn->setContext(context);
    } else loop_->quit();
}

private:
    bool parse(std::string& buf, const ContextPtr& context)
    {
        bool isComplete = true;
        while (isComplete) {
            auto it = std::search(buf.begin(), buf.end(), kCRLF.begin(), kCRLF.end());
            if (it != buf.end()) {
                context->state = 1;
                std::string str = buf.substr(0, std::distance(buf.begin(), it));
                if (context->count == 0 && str[0] == '*') {
                    context->count = std::stoi(str.substr(1));
                } else if (context->length == 0 && str[0] == '$') {
                    context->length = std::stoi(str.substr(1));
                } else if (context->length > 0) {
                    if (str.size() != context->length) return false;
                    context->cmds.push_back(str);
                    context->length = 0;
                    context->count--;
                    if (context->count == 0) context->state = 2;
                }
                buf.erase(buf.begin(), it+2);
            } else {
                isComplete = false;
            }
        }
        return true;
    }   

    slite::EventLoop* loop_;
    slite::TCPServer server_;
    std::map<std::string, std::string> db_;
};



int main()
{
    slite::Logger::setLogLevel(slite::Logger::DEBUG);
    MemoryDBServer server("127.0.0.1", 6379);
    server.start();
}
