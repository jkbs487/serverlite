#include "TCPServer.h"
#include "TCPClient.h"
#include "EventLoop.h"
#include "Logger.h"
#include "IM.Login.pb.h"
#include "IM.Server.pb.h"
#include "IM.Other.pb.h"

#include "../protobuf/codec.h"
#include "../protobuf/dispatcher.h"

#include <set>
#include <memory>
#include <functional>
#include <sys/time.h>

using namespace tcpserver;
using namespace std::placeholders;

class MsgServer
{
public:
    MsgServer(std::string host, uint16_t port, EventLoop* loop);
    ~MsgServer();

    void start() { 
        server_.start(); 
        client_.connect();
    }
    void onTimer();

private:
    void onConnection(const TCPConnectionPtr& conn);
    void onClientConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);
    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onMsgServInfo(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onUserCntUpdate(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onMsgServRequest(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
    };

    TCPServer server_;
    TCPClient client_;
    EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
    std::set<TCPConnectionPtr> clientConns_;
    std::set<TCPConnectionPtr> msgConns_;
    int total_online_user_cnt_;
};

MsgServer::MsgServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "MsgServer"),
    client_(host, 12345, loop, "MsgClient"),
    loop_(loop),
    dispatcher_(std::bind(&MsgServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    server_.setConnectionCallback(
        std::bind(&MsgServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    client_.setConnectionCallback(
        std::bind(&MsgServer::onClientConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&MsgServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&MsgServer::onHeartBeat, this, _1, _2, _3));

    //loop_->runEvery(1.0, std::bind(&MsgServer::onTimer, this));
}

MsgServer::~MsgServer()
{
}

void MsgServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto msgConn : msgConns_) {
        Context context = std::any_cast<Context>(msgConn->getContext());

        if (currTick > context.lastSendTick + 5000) {
            IM::Other::IMHeartBeat msg;
            codec_.send(msgConn, msg);
        }
        
        if (currTick > context.lastRecvTick + 30000) {
            LOG_ERROR << "Connect to MsgServer timeout";
            msgConn->forceClose();
        }
    }
}

void MsgServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        msgConns_.insert(conn);
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);
    } else {
        msgConns_.erase(conn);
    }
}

void MsgServer::onClientConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        msgConns_.insert(conn);
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
        msg.set_port(10002);
        msg.set_cur_conn_cnt(0);
        msg.set_max_conn_cnt(10);
        codec_.send(conn, msg);
    } else {
        msgConns_.erase(conn);
    }
}

void MsgServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    LOG_DEBUG << "MsgServer::onMessage " << buffer;
}

void MsgServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void MsgServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void MsgServer::onHeartBeat(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
    codec_.send(conn, *message.get());
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    MsgServer msgServer("0.0.0.0", 10002, &loop);
    msgServer.start();
    loop.loop();
}