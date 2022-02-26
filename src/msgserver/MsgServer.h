#include "slite/TCPConnection.h"
#include "slite/TCPServer.h"
#include "slite/TCPClient.h"
#include "slite/EventLoop.h"

#include "pbs/IM.Login.pb.h"
#include "pbs/IM.Server.pb.h"
#include "pbs/IM.Buddy.pb.h"
#include "pbs/IM.Other.pb.h"
#include "pbs/IM.Group.pb.h"

#include "slite/protobuf/codec.h"
#include "slite/protobuf/dispatcher.h"

#include <set>
#include <memory>
#include <functional>

typedef std::shared_ptr<IM::Login::IMLoginReq> LoginReqPtr;
typedef std::shared_ptr<IM::Login::IMLogoutReq> LogoutReqPtr;
typedef std::shared_ptr<IM::Server::IMValidateRsp> ValidateRspPtr;
typedef std::shared_ptr<IM::Buddy::IMDepartmentReq> DepartmentReqPtr;
typedef std::shared_ptr<IM::Buddy::IMDepartmentRsp> DepartmentRspPtr;
typedef std::shared_ptr<IM::Buddy::IMAllUserReq> AllUserReqPtr;
typedef std::shared_ptr<IM::Buddy::IMAllUserRsp> AllUserRspPtr;
typedef std::shared_ptr<IM::Buddy::IMRecentContactSessionReq> RecentContactSessionReqPtr;
typedef std::shared_ptr<IM::Buddy::IMUsersStatReq> UsersStatReqPtr;

typedef std::shared_ptr<IM::Group::IMNormalGroupListReq> NormalGroupListReqPtr;
typedef std::shared_ptr<IM::Group::IMGroupChangeMemberRsp> GroupChangeMemberRspPtr;

class MsgServer
{
public:
    MsgServer(std::string host, uint16_t port, EventLoop* loop);
    ~MsgServer();

    void start() { 
        server_.start(); 
    }
    void onTimer();

private:
    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);

    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
   
    void onLoginRequest(const TCPConnectionPtr& conn, const LoginReqPtr& message, int64_t receiveTime);
    void onLoginOutRequest(const TCPConnectionPtr& conn, const LogoutReqPtr& message, int64_t receiveTime);
    
    void onClientDepartmentRequest(const TCPConnectionPtr& conn, const DepartmentReqPtr& message, int64_t receiveTime);
    void onRecentContactSessionRequest(const TCPConnectionPtr& conn, const RecentContactSessionReqPtr& message, int64_t receiveTime);
    void onAllUserRequest(const TCPConnectionPtr& conn, const AllUserReqPtr& message, int64_t receiveTime);
    void onUsersStatusRequest(const TCPConnectionPtr& conn, const UsersStatReqPtr& message, int64_t receiveTime);
    
    void onNormalGroupListRequest(const TCPConnectionPtr& conn, const NormalGroupListReqPtr& message, int64_t receiveTime);
    void onGroupChangeMemberResponse(const TCPConnectionPtr& conn, const GroupChangeMemberRspPtr& message, int64_t receiveTime);

    static const int heartBeatInterVal = 5000;
    static const int timeout = 30000;

    TCPServer server_;
    EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

class LoginClient
{
public:
    LoginClient(std::string host, uint16_t port, EventLoop* loop);
    ~LoginClient() {}

    void connect() {
        client_.connect();
    }
private:
    void onTimer();
    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);
    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);

    TCPClient client_;
    EventLoop* loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};

void LoginClient::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    // do nothing
    return ;
}

class DBProxyClient
{
public:
    DBProxyClient(std::string host, uint16_t port, EventLoop* loop);
    ~DBProxyClient() {}

    void connect() {
        client_.connect();
    }
private:
    void onTimer();
    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);
    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onValidateResponseRequest(const TCPConnectionPtr& conn, const ValidateRspPtr& message, int64_t receiveTime);
    void onClientDepartmentResponse(const TCPConnectionPtr& conn, const DepartmentRspPtr& message, int64_t receiveTime);

    void onClientAllUserResponse(const TCPConnectionPtr& conn, const AllUserRspPtr& message, int64_t receiveTime);

    TCPClient client_;
    EventLoop* loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};