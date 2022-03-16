#include "LoginServer.h"

#include "slite/Logger.h"
#include "nlohmann/json.hpp"

#include <sys/time.h>

using namespace slite;
using namespace std::placeholders;

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
        std::bind(&LoginServer::onMessage, this, _1, _2, _3));
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
    codec_.onMessage(conn, buffer, receiveTime);
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
    //LOG_INFO << "onHeartBeat[" << conn->name() << "]: " << message->GetTypeName();
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
	LOG_INFO << "MsgServInfo, ip_addr1=" << message->ip1() << ", ip_addr2=" << message->ip2() 
        << ", port=" << message->port() << ", max_conn_cnt=" << message->max_conn_cnt() 
        << ", cur_conn_cnt= " << message->cur_conn_cnt() << ", hostname: " << message->host_name();
}

void LoginServer::onUserCntUpdate(const TCPConnectionPtr& conn, 
                                const UserCntUpdatePtr& message, 
                                int64_t receiveTime)
{
    MsgServInfo* info = 
        std::any_cast<Context*>(conn->getContext())->msgServInfo;
    
    if (message->user_action() == 1) {
        info->curConnCnt++;
        totalOnlineUserCnt_++;
    } else {
        if (info->curConnCnt > 0)
            info->curConnCnt--;
        if (totalOnlineUserCnt_ > 0)
            totalOnlineUserCnt_--;
    }

    LOG_INFO << "onUserCntUpdate: " << info->hostname << ":" << info->port << ", curCnt=" 
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