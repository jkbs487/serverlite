#include "slite/TCPConnection.h"
#include "slite/TCPServer.h"
#include "slite/TCPClient.h"
#include "slite/EventLoop.h"
#include "slite/protobuf/codec.h"
#include "slite/protobuf/dispatcher.h"

#include "base/messagePtr.h"
#include "UserManager.h"

#include <set>
#include <memory>
#include <functional>

typedef std::map<uint32_t, IM::UserManager*> UserManagerMap;

namespace IM {

class RouteServer
{
public:
    RouteServer(std::string host, uint16_t port, slite::EventLoop* loop);
    ~RouteServer();

    void start() { server_.start(); }
    std::string host() { return server_.host(); }
    uint16_t port() { return server_.port(); }

    void onTimer();

private:

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
        bool master;
    };

    void onConnection(const slite::TCPConnectionPtr& conn);
    void onMessage(const slite::TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const slite::TCPConnectionPtr& conn);

    void onUnknownMessage(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onDefaultMessage(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
   
    void onOnlineUserInfo(const slite::TCPConnectionPtr& conn, const OnlineUserInfoPtr& message, int64_t receiveTime);
    void onUserStatusUpdate(const slite::TCPConnectionPtr& conn, const UserStatusUpdatePtr& message, int64_t receiveTime);
    void onRoleSet(const slite::TCPConnectionPtr& conn, const RoleSetPtr& message, int64_t receiveTime);
    void onUsersStatusRequest(const slite::TCPConnectionPtr& conn, const UsersStatReqPtr& message, int64_t receiveTime);

    UserManager* getUserInfo(uint32_t userId);
    void broadcastMsg(const google::protobuf::Message& message, const slite::TCPConnectionPtr& fromConn = nullptr);
    void updateUserStatus(const slite::TCPConnectionPtr& conn, uint32_t userId, uint32_t status, uint32_t clientType);

    slite::TCPServer server_;
    slite::EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    slite::ProtobufCodec codec_;
    std::set<slite::TCPConnectionPtr> clientConns_;
    UserManagerMap userManagerMap_;

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;
};

}