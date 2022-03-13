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

class MsgServer
{
public:
    MsgServer(std::string host, uint16_t port, slite::EventLoop* loop);
    ~MsgServer();

    void start() { server_.start(); }
    std::string host() { return server_.host(); }
    uint16_t port() { return server_.port(); }

    void onTimer();

private:
    void onConnection(const slite::TCPConnectionPtr& conn);
    void onMessage(const slite::TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const slite::TCPConnectionPtr& conn);

    void onUnknownMessage(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
   
    void onLoginRequest(const slite::TCPConnectionPtr& conn, const LoginReqPtr& message, int64_t receiveTime);
    void onLoginOutRequest(const slite::TCPConnectionPtr& conn, const LogoutReqPtr& message, int64_t receiveTime);
    
    void onClientDepartmentRequest(const slite::TCPConnectionPtr& conn, const DepartmentReqPtr& message, int64_t receiveTime);
    void onRecentContactSessionRequest(const slite::TCPConnectionPtr& conn, const RecentContactSessionReqPtr& message, int64_t receiveTime);
    void onAllUserRequest(const slite::TCPConnectionPtr& conn, const AllUserReqPtr& message, int64_t receiveTime);
    void onUsersStatusRequest(const slite::TCPConnectionPtr& conn, const UsersStatReqPtr& message, int64_t receiveTime);
    
    void onNormalGroupListRequest(const slite::TCPConnectionPtr& conn, const NormalGroupListReqPtr& message, int64_t receiveTime);
    void onGroupChangeMemberResponse(const slite::TCPConnectionPtr& conn, const GroupChangeMemberRspPtr& message, int64_t receiveTime);

    void onMsgData(const slite::TCPConnectionPtr& conn, const MsgDataPtr& message, int64_t receiveTime);
    void onUnreadMsgCntRequest(const slite::TCPConnectionPtr& conn, const UnreadMsgCntReqPtr& message, int64_t receiveTime);
    void onGetMsgListRequest(const slite::TCPConnectionPtr& conn, const GetMsgListReqPtr& message, int64_t receiveTime);

    void onFileHasOfflineRequest(const slite::TCPConnectionPtr& conn, const FileHasOfflineReqPtr& message, int64_t receiveTime);

    void onP2PCmdMsg(const slite::TCPConnectionPtr& conn, const P2PCmdMsgPtr& message, int64_t receiveTime);

    slite::TCPServer server_;
    slite::EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    slite::ProtobufCodec codec_;
    IM::ProtobufCodec clientCodec_;

    static const int heartBeatInterVal = 5000;
    static const int timeout = 30000;
};

}