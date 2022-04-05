#include "slite/TCPClient.h"
#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"
#include "slite/ThreadPool.h"

#include <any>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <getopt.h>
#include <assert.h>

using namespace slite;
std::mutex mutex_;
EventLoop* g_loop;
ThreadPool* g_pool;
int g_receiveCount = 0;
TCPConnectionPtr g_publishConn;
//thread_local TCPConnectionPtr g_conns;


std::map<std::string, std::vector<TCPConnectionPtr>> conns;

std::string generageLongString()
{
    return "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
        bbbbbbbbbbbbbbbbbbbbbbbbbbbbb\
        ccccccccccccccccccccccccccccc\
        ddddddddddddddddddddddddddddd\
        eeeeeeeeeeeeeeeeeeeeeeeeeeeeee\
        ffffffffffffffffffffffffffffff\
        gggggggggggggggggggggggggggggg";
}

std::string strip(std::string str)
{
    for (auto it = str.begin(); it != str.end(); ) {
        if (*it == '\r' || *it == '\n' || *it == ' ') {
            it = str.erase(it);
        } else {
            break;
        }
    }

    for (auto it = str.rbegin(); it != str.rend(); ) {
        if (*it == '\r' || *it == '\n' || *it == ' ') {
            str.erase(--(it++).base());
        } else {
            break;
        }
    }
    return str;
}

std::string getMessage(std::string& buffer, int &errcode)
{
    errcode = 0;
    size_t pos = buffer.find_first_of("\r\n");
    if (pos == std::string::npos) {
        errcode = 1;
        buffer.erase(buffer.begin(), buffer.end());
        return "";
    }
    std::string data = buffer.substr(0, pos);
    if (data.size() > 100 || data.size() < 5) {
        errcode = 2;
    }
    buffer.erase(buffer.begin(), buffer.begin() + pos + 2);
    return data;
}

void onServerMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    while (buffer.find("\r\n") != std::string::npos) {
        int errcode = 0;
        std::string data = getMessage(buffer, errcode);
        if (errcode == 2) {
            LOG_ERROR << conn->name() << " content length is invalid";
            continue;;
        }
        if (data.substr(0, 3) == "sub") {
            std::string topic = strip(data.substr(4));
            std::set<std::string> topics = std::any_cast<std::set<std::string>>(conn->getContext());
            topics.insert(topic);
            conn->setContext(topics);
            LOG_INFO << conn->name() << " subscribe topic [" << topic << "]";
            {
                std::lock_guard<std::mutex> lock(mutex_);
                conns[topic].push_back(conn);
            }
        } else if (data.substr(0, 3) == "pub") {
            size_t pos = data.find_first_of(' ');
            std::string topic = strip(data.substr(4, pos-4));
            std::string content = strip(data.substr(pos+1, data.size()+1));
            LOG_INFO << conn->name() << " publish topic [" << topic << "] content [" << content << "]";
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (conns.count(topic) == 0) {
                    LOG_INFO << "no topic " << topic;
                    return;
                }
                for (const auto& connection : conns[topic]) {
                    //g_pool->run([&]{
                        connection->send(topic + ":" + content + "\r\n");
                    //});
                }
            }
        } else if (data.substr(0, 5) == "unsub") {
            std::string topic = strip(data.substr(6));
            LOG_INFO << conn->peerAddr() << ":" << conn->peerPort()
            << " unsub topic " << topic;
            std::set<std::string> topics = std::any_cast<std::set<std::string>>(conn->getContext());
            topics.erase(topic);
            conn->setContext(topics);
            if (conns.count(topic) == 0) return;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = std::find(conns[topic].begin(), conns[topic].end(), conn);
                conns[topic].erase(it);
            }
        } else {
            LOG_ERROR << "unknow message";
            conn->forceClose();
        }
    }
}

void onServerConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        conn->setContext(std::set<std::string>());
    } else {
        std::set<std::string> topics = std::any_cast<std::set<std::string>>(conn->getContext());
        for (auto topic: topics) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = std::find(conns[topic].begin(), conns[topic].end(), conn);
            conns[topic].erase(it);
        }
    }
}

void onClientConnection(const TCPConnectionPtr& conn)
{
    if(conn->connected()) {
        conn->send("sub:foo\r\n");
        conn->send("sub:bar\r\n");
    } else {
        conn->send("unsub:foo\r\n");
        conn->send("unsub:bar\r\n");
    }
}

void onClientMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    int errcode = 0;
    while (buffer.find("\r\n") != std::string::npos) {
        if (errcode == 2) {
            LOG_ERROR << "content length is invalid";
            continue;
        }
        std::string data = getMessage(buffer, errcode);
        size_t pos = data.find_first_of(':');
        if (pos == std::string::npos) continue;;
        std::string topic = strip(data.substr(0, pos));
        std::string msg = strip(data.substr(pos+1));
        LOG_INFO << conn->name() << " recv msg[" << topic << "]: " << msg;
        if (topic == "foo") assert(msg == "hello");
        else if (topic == "bar") assert(msg == "world");
        g_receiveCount++;
    }
}

void onClientConnection2(const TCPConnectionPtr& conn)
{
    if (conn->connected()) 
        g_publishConn = conn;
}

void usage() {
    printf("Usage: -s -c\n");
    printf("-s: Server Patten\n");
    printf("-c: Client Patten\n");
}

int main(int argc, char **argv)
{
    bool serverPatten;
    int ret = getopt(argc, argv, "cs");
    switch (ret)
    {
    case 'c':
        serverPatten = false; 
        break;
    case 's':
        serverPatten = true;
        break;
    default:
        usage();
        return 0;
    }

    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    ThreadPool pool;
    g_loop = &loop;
    g_pool = &pool;
    
    if (!serverPatten) {
        std::vector<TCPClient*> subcribers;
        for (int i = 0; i < 1000; i++) {
            std::string name = "sub" + std::to_string(i+1);
            subcribers.emplace_back(new TCPClient("127.0.0.1", 12345, &loop, name));
        }
        for (auto sub: subcribers) {
            sub->setConnectionCallback(onClientConnection);
            sub->setMessageCallback(onClientMessage);
            sub->connect();
        }
        TCPClient publisher("127.0.0.1", 12345, &loop, "pub");
        publisher.setMessageCallback(onClientMessage);
        publisher.setConnectionCallback(onClientConnection2);
        publisher.connect();
        loop.runEvery(4, [&]{ g_publishConn->send("pub:foo hello\r\n"); });
        loop.runEvery(6, [&]{ g_publishConn->send("pub:bar world\r\n"); });
        loop.runEvery(10, [&]{ LOG_DEBUG << "receive count: " << g_receiveCount; });
        loop.loop();
    } else {
        pool.start(4); 
        TCPServer server("127.0.0.1", 12345, &loop, "SubPubServer");
        server.setMessageCallback(onServerMessage);
        server.setConnectionCallback(onServerConnection);
        server.setThreadNum(4);
        server.start();
        loop.loop();
    }
}