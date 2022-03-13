#include "MsgServer.h"
#include "LoginClient.h"
#include "DBProxyClient.h"
#include "RouteClient.h"

#include "slite/Logger.h"

IM::MsgServer* g_msgServer;

std::set<slite::TCPConnectionPtr> g_clientConns;
std::set<slite::TCPConnectionPtr> g_loginConns;
std::set<slite::TCPConnectionPtr> g_dbProxyConns;
std::set<slite::TCPConnectionPtr> g_routeConns;
std::set<slite::TCPConnectionPtr> g_fileConns;

int main(int argc, char* argv[])
{
    if (argc == 2) {
        slite::Logger::setLogLevel(slite::Logger::DEBUG);
        slite::EventLoop loop;
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