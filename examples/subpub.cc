#include "TCPClient.h"
#include "TCPServer.h"
#include "EventLoop.h"
#include "Logger.h"

#include <any>
#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <algorithm>

using namespace slite;

std::mutex mutex_;
EventLoop* g_loop;
int g_receiveCount = 0;

std::map<std::string, std::vector<TCPConnectionPtr>> conns;

void onServerMessage(const TCPConnectionPtr& conn, std::string buffer, int64_t receiveTime)
{
    std::string data;
    data = std::move(buffer);
    if (data.find("sub:") != std::string::npos) {
        std::string topic = data.substr(4, data.size());
        std::set<std::string> topics = std::any_cast<std::set<std::string>>(conn->getContext());
        topics.insert(topic);
        conn->setContext(topics);
        LOG_INFO << conn->peerAddr() << ":" << conn->peerPort()
         << " sub topic " << topic;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            conns[topic].push_back(conn);
        }
    } else if (data.find("pub:") != std::string::npos) {
        size_t pos = data.find_first_of(' ');
        std::string topic = data.substr(4, pos-4);
        std::string content = data.substr(pos+1, content.size());
        LOG_INFO << conn->peerAddr() << ":" << conn->peerPort() 
            << " pub topic " << topic << " content " << content;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (conns.count(topic) == 0) {
                LOG_INFO << "no topic " << topic;
                return;
            }
            for (const auto& connection : conns[topic]) {
                LOG_DEBUG << "conn use count: " << connection.use_count();
                connection->send(content);
            }
        }
    } else if (data.find("unsub:") != std::string::npos) {
        std::string topic = data.substr(4, data.size());
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
        conn->send("sub:test");
    }
}

void onClientMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    LOG_INFO << "[" << conn->name() << "] " << buffer;
    std::string().swap(buffer);
    g_receiveCount++;
    conn->shutdown();
}

void onClientConnection2(const TCPConnectionPtr& conn)
{
    if (conn->connected()) 
        g_loop->runAfter(10, [&](){
            conn->send("pub:test test"); 
            conn->shutdown();
        });
    //conn->shutdown();
}

int main(int argc, char **argv)
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    g_loop = &loop;
    if (argc > 1) {
        std::vector<TCPClient*> subcribers;
        for (int i = 0; i < 10000; i++) {
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
        loop.runAfter(12, [&](){ LOG_DEBUG << "receive count: " << g_receiveCount; });
        loop.loop();
    } else {
        TCPServer server("127.0.0.1", 12345, &loop, "SubPubServer");
        server.setMessageCallback(onServerMessage);
        server.setConnectionCallback(onServerConnection);
        server.setThreadNum(4);
        server.start();
        loop.loop();
    }
}