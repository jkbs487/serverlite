#include "TCPServer.h"
#include "EventLoop.h"
#include "Logger.h"
#include "IM.Login.pb.h"
#include "IM.Server.pb.h"
#include "IM.Other.pb.h"

#include "../http/HTTPCodec.h"
#include "../protobuf/codec.h"
#include "../protobuf/dispatcher.h"
#include "nlohmann/json.hpp"

#include <set>
#include <memory>
#include <functional>
#include <sys/time.h>

using namespace slite;
using namespace std::placeholders;

typedef std::shared_ptr<IM::Server::IMMsgServInfo> MsgServInfoPtr;
typedef std::shared_ptr<IM::Server::IMUserCntUpdate> UserCntUpdatePtr;
typedef std::shared_ptr<IM::Login::IMMsgServReq> MsgServReqPtr;
typedef std::shared_ptr<IM::Other::IMHeartBeat> HeartBeatPtr;

class LoginServer
{
public:
    LoginServer(std::string host, uint16_t port, EventLoop* loop);
    ~LoginServer();

    void start() { 
        server_.start(); 
        httpServer_.start(); 
    }
    void onTimer();

private:
    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);
    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const HeartBeatPtr& message, int64_t receiveTime);
    void onMsgServInfo(const TCPConnectionPtr& conn, const MsgServInfoPtr& message, int64_t receiveTime);
    void onUserCntUpdate(const TCPConnectionPtr& conn, const UserCntUpdatePtr& message, int64_t receiveTime);
    void onMsgServRequest(const TCPConnectionPtr& conn, const MsgServReqPtr& message, int64_t receiveTime);
    
    HTTPResponse onHttpRequest(HTTPRequest* req);
    HTTPResponse onHttpMsgServRequest();


    struct MsgServInfo {
        std::string	ipAddr1;	// 电信IP
        std::string	ipAddr2;	// 网通IP
        uint16_t port;
        uint32_t maxConnCnt;
        uint32_t curConnCnt;
        std::string hostname;	// 消息服务器的主机名
    };

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
        MsgServInfo* msgServInfo;
    };

    TCPServer server_;
    TCPServer httpServer_;
    EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
    HTTPCodec httpCodec_;
    std::set<TCPConnectionPtr> clientConns_;
    std::set<TCPConnectionPtr> msgConns_;
    int totalOnlineUserCnt_;

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};

LoginServer::LoginServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "LoginServer"),
    httpServer_(host, 8001, loop, "HttpServer"),
    loop_(loop),
    dispatcher_(std::bind(&LoginServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    httpCodec_(std::bind(&LoginServer::onHttpRequest, this, _1)),
    totalOnlineUserCnt_(0)
{
    server_.setConnectionCallback(
        std::bind(&LoginServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&LoginServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&LoginServer::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMMsgServInfo>(
        std::bind(&LoginServer::onMsgServInfo, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMUserCntUpdate>(
        std::bind(&LoginServer::onUserCntUpdate, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Login::IMMsgServReq>(
        std::bind(&LoginServer::onMsgServRequest, this, _1, _2, _3));
    httpServer_.setMessageCallback(
        std::bind(&HTTPCodec::onMessage, &httpCodec_, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&LoginServer::onTimer, this));
}

LoginServer::~LoginServer()
{
}

void LoginServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto msgConn : msgConns_) {
        Context* context = std::any_cast<Context*>(msgConn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(msgConn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to MsgServer timeout";
            msgConn->forceClose();
        }
    }
}

HTTPResponse LoginServer::onHttpRequest(HTTPRequest* req)
{
    HTTPResponse resp;
    if (req->path() == "/msg_server") {
        resp = onHttpMsgServRequest();
    } else {
        resp.setStatus(HTTPResponse::BAD_REQUEST);
    }
    return resp;
}

HTTPResponse LoginServer::onHttpMsgServRequest()
{
    HTTPResponse resp;
    
    if (msgConns_.empty()) {
        std::string body = "{\"code\": 1, \"msg\": \"消息服务器不存在\"}";
        resp.setContentLength(body.size());
        resp.setBody(body);
        return resp;
    }

    uint32_t minUserCnt = static_cast<uint32_t>(-1); 
    TCPConnectionPtr minMsgConn = nullptr;

    for (auto msgConn: msgConns_) {
        Context* context = std::any_cast<Context*>(msgConn->getContext());
        MsgServInfo* info = context->msgServInfo;
        if ((info->curConnCnt < info->maxConnCnt) && 
            (info->curConnCnt < minUserCnt)) {
            minMsgConn = msgConn;
            minUserCnt = info->curConnCnt;
        }
    }

    if (minMsgConn == nullptr) {
        LOG_ERROR << "All TCP MsgServer are full";
        nlohmann::json body = nlohmann::json::object();
        body["code"] = 2;
        body["msg"] = "负载过高";
        resp.setContentLength(body.dump().size());
        resp.setBody(body.dump());
    } else {
        MsgServInfo* info = std::any_cast<Context*>(
            minMsgConn->getContext())->msgServInfo;
        nlohmann::json body = nlohmann::json::object();
        body["code"] = 0;
        body["msg"] = "OK";
        body["priorIP"] = info->ipAddr1;
        body["backupIP"] = info->ipAddr2;
        body["msfsPrior"] = "http://127.0.0.1:8700/";
        body["msfsBackup"] = "http://127.0.0.1:8700/";
        body["discovery"] = "http://127.0.0.1/api/discovery";
        body["port"] = std::to_string(info->port);
        resp.setContentLength(body.dump().size());
        LOG_DEBUG << body.dump();
        resp.setBody(body.dump());
    }

    resp.setStatus(HTTPResponse::OK);
    resp.setContentType("text/html;charset=utf-8");
    resp.setHeader("Connection", "close");
    return resp;
}

void LoginServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        msgConns_.insert(conn);
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        context->msgServInfo = nullptr;
        conn->setContext(context);
    } else {
        Context* context = std::any_cast<Context*>(conn->getContext());
        delete context;
        msgConns_.erase(conn);
    }
}

void LoginServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void LoginServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void LoginServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void LoginServer::onHeartBeat(const TCPConnectionPtr& conn,
                                const HeartBeatPtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onHeartBeat[" << conn->name() << "]: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void LoginServer::onMsgServInfo(const TCPConnectionPtr& conn,
                                const MsgServInfoPtr& message,
                                int64_t)
{
    LOG_INFO << "onMsgServInfo: " << message->GetTypeName();
    MsgServInfo* msgServInfo = new MsgServInfo();
    msgServInfo->ipAddr1 = message->ip1();
    msgServInfo->ipAddr2 = message->ip2();
    msgServInfo->port = static_cast<uint16_t>(message->port());
    msgServInfo->hostname = message->host_name();
    msgServInfo->maxConnCnt = message->max_conn_cnt();
    msgServInfo->curConnCnt = message->cur_conn_cnt();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->msgServInfo = msgServInfo;
}

void LoginServer::onUserCntUpdate(const TCPConnectionPtr& conn, 
                                const UserCntUpdatePtr& message, 
                                int64_t receiveTime)
{
    LOG_INFO << "onUserCntUpdate: " << message->GetTypeName();
    MsgServInfo* info = 
        std::any_cast<Context*>(conn->getContext())->msgServInfo;
    
    if (message->user_action() == 1) {
        info->curConnCnt++;
        totalOnlineUserCnt_++;
    } else {
        info->curConnCnt--;
        totalOnlineUserCnt_--;
    }

    LOG_INFO << info->hostname << ":" << info->port << ", curCnt=" 
    << info->curConnCnt << ", totalCnt=" << totalOnlineUserCnt_;
}

void LoginServer::onMsgServRequest(const TCPConnectionPtr& conn, 
                                const MsgServReqPtr& message, 
                                int64_t receiveTime)
{
    LOG_INFO << "onMsgServRequest: " << message->GetTypeName();
    
    if (msgConns_.empty()) {
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NO_MSG_SERVER);
        codec_.send(conn, msg);
        return;
    }

    uint32_t minUserCnt = static_cast<uint32_t>(-1); 
    TCPConnectionPtr minMsgConn = nullptr;

    for (auto msgConn: msgConns_) {
        Context* context = std::any_cast<Context*>(msgConn->getContext());
        MsgServInfo* info = context->msgServInfo;
        if ((info->curConnCnt < info->maxConnCnt) && 
            (info->curConnCnt < minUserCnt)) {
            minMsgConn = msgConn;
            minUserCnt = info->curConnCnt;
        }
    }

    if (minMsgConn == nullptr) {
        LOG_WARN << "All TCP MsgServer are full";
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_MSG_SERVER_FULL);
        codec_.send(conn, msg);
    } else {
        Context* context = std::any_cast<Context*>(minMsgConn->getContext());
        MsgServInfo* info = context->msgServInfo;
        IM::Login::IMMsgServRsp msg;
        msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NONE);
        msg.set_prior_ip(info->ipAddr1);
        msg.set_backip_ip(info->ipAddr2);
        msg.set_port(info->port);
        codec_.send(conn, msg);
    }

    // after send MsgServResponse, active close the connection
    conn->shutdown();
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    LoginServer loginServer("0.0.0.0", 10001, &loop);
    loginServer.start();
    loop.loop();
}