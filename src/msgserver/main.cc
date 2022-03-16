#include "MsgServer.h"
#include "LoginClient.h"
#include "DBProxyClient.h"
#include "RouteClient.h"

#include "slite/Logger.h"

#include <random>

using namespace slite;

uint32_t g_downMsgMissCnt = 0; // 下行消息丢包数
uint32_t g_downMsgTotalCnt = 0;	// 下行消息包总数
uint32_t g_upMsgTotalCnt = 0; // 上行消息包总数
uint32_t g_upMsgMissCnt = 0; // 上行消息包丢数
IM::MsgServer* g_msgServer;

std::set<slite::TCPConnectionPtr> g_clientConns;
std::set<slite::TCPConnectionPtr> g_loginConns;
std::set<slite::TCPConnectionPtr> g_dbProxyConns;
std::set<slite::TCPConnectionPtr> g_routeConns;
std::set<slite::TCPConnectionPtr> g_fileConns;

TCPConnectionPtr getRandomConn(std::set<TCPConnectionPtr> conns, size_t start, size_t end)
{
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned long> u(start, end);
    while (true && !conns.empty()) {
        auto it = conns.begin();
        if (it == conns.end()) return nullptr;
        std::advance(it, u(e));
        if (it != conns.end())
            return *it;
    }
    return nullptr;
}

TCPConnectionPtr getRandomDBProxyConnForLogin()
{
    return getRandomConn(g_dbProxyConns, 0, g_dbProxyConns.size()-1);
}

TCPConnectionPtr getRandomDBProxyConn()
{
    return getRandomConn(g_dbProxyConns, g_dbProxyConns.size()/2, g_dbProxyConns.size()-1);
}

TCPConnectionPtr getRandomRouteConn()
{
    return getRandomConn(g_routeConns, 0, g_routeConns.size()-1);
}

int main(int argc, char* argv[])
{
    if (argc == 2) {
        Logger::setLogLevel(Logger::DEBUG);
        EventLoop loop;
        IM::MsgServer msgServer("0.0.0.0", static_cast<uint16_t>(atoi(argv[1])), &loop);
        g_msgServer = &msgServer;
        IM::LoginClient loginClient("127.0.0.1", 10001, &loop);
        IM::DBProxyClient dbProxyClient("127.0.0.1", 10003, &loop);
        IM::RouteClient routeClient("127.0.0.1", 10004, &loop);
        msgServer.start();
        loginClient.connect();
        dbProxyClient.connect();
        routeClient.connect();
        loop.loop();
    }
}