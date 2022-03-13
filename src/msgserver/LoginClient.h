#pragma once

#include "slite/TCPConnection.h"
#include "slite/TCPServer.h"
#include "slite/TCPClient.h"
#include "slite/EventLoop.h"

#include "base/messagePtr.h"
#include "base/protobuf_codec.h"
#include "slite/protobuf/codec.h"

#include "slite/protobuf/dispatcher.h"

#include <set>
#include <memory>
#include <functional>

extern std::set<slite::TCPConnectionPtr> g_clientConns;
extern std::set<slite::TCPConnectionPtr> g_loginConns;
extern std::set<slite::TCPConnectionPtr> g_dbProxyConns;
extern std::set<slite::TCPConnectionPtr> g_routeConns;
extern std::set<slite::TCPConnectionPtr> g_fileConns;

namespace IM {

class LoginClient
{
public:
    LoginClient(std::string host, uint16_t port, slite::EventLoop* loop);
    ~LoginClient() {}

    void connect() {
        client_.connect();
    }
private:
    void onTimer();
    void onConnection(const slite::TCPConnectionPtr& conn);
    void onMessage(const slite::TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const slite::TCPConnectionPtr& conn);
    void onUnknownMessage(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);

    slite::TCPClient client_;
    slite::EventLoop* loop_;
    ProtobufDispatcher dispatcher_;
    slite::ProtobufCodec codec_;
    IM::ProtobufCodec clientCodec_;

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
    };

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};

}