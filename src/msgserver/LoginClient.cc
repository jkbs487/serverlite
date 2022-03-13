#include "LoginClient.h"
#include "MsgServer.h"
#include "slite/Logger.h"

#include <sys/time.h>

using namespace IM;
using namespace slite;
using namespace std::placeholders;

extern MsgServer* g_msgServer;

LoginClient::LoginClient(std::string host, uint16_t port, EventLoop* loop)
    : client_(host, port, loop, "LoginClient"),
    loop_(loop),
    dispatcher_(std::bind(&LoginClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    clientCodec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    client_.setConnectionCallback(
        std::bind(&LoginClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&LoginClient::onMessage, this, _1, _2, _3));
    client_.setWriteCompleteCallback(
        std::bind(&LoginClient::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&LoginClient::onHeartBeat, this, _1, _2, _3));
    loop_->runEvery(1.0, std::bind(&LoginClient::onTimer, this));
}

void LoginClient::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        LOG_INFO << "connect login server success";
        g_loginConns.insert(conn);
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);

        IM::Server::IMMsgServInfo msg;
        msg.set_ip1("192.168.142.128");
        msg.set_ip2("192.168.142.128");
        msg.set_host_name("msgserver");
        msg.set_port(g_msgServer->port());
        msg.set_cur_conn_cnt(0);
        msg.set_max_conn_cnt(10);
        codec_.send(conn, msg);
    } else {
        g_loginConns.erase(conn);
    }
}

void LoginClient::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    codec_.onMessage(conn, buffer, receiveTime);
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void LoginClient::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void LoginClient::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (const auto& conn : g_loginConns) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to LoginServer timeout";
            conn->forceClose();
        }
    }
}

void LoginClient::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_ERROR << "onUnknownMessage: " << message->GetTypeName();
    //conn->shutdown();
}

void LoginClient::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    // do nothing
    return ;
}