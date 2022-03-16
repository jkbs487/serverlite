#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/http/HTTPCodec.h"
#include "slite/protobuf/codec.h"
#include "slite/protobuf/dispatcher.h"

#include "pbs/IM.Login.pb.h"
#include "pbs/IM.Server.pb.h"
#include "pbs/IM.Other.pb.h"

#include <set>
#include <memory>
#include <functional>

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
    slite::ProtobufCodec codec_;
    HTTPCodec httpCodec_;
    std::set<TCPConnectionPtr> clientConns_;
    std::set<TCPConnectionPtr> msgConns_;
    int totalOnlineUserCnt_;

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};