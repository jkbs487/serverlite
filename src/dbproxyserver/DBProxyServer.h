#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/ThreadPool.h"
#include "slite/protobuf/codec.h"
#include "slite/protobuf/dispatcher.h"

#include "base/messagePtr.h"
#include "models/DBPool.h"
#include "models/CachePool.h"
#include "models/DepartmentModel.h"
#include "models/UserModel.h"
#include "SyncCenter.h"

#include <set>
#include <memory>
#include <functional>

namespace IM {

class DBProxyServer
{
public:
    DBProxyServer(std::string host, uint16_t port, slite::EventLoop* loop);
    ~DBProxyServer();

    void start() {
        threadPool_.start(4);
        server_.start();
    }
    void onTimer();

private:
    DBProxyServer(const DBProxyServer& server) = delete;

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
    };

    void onConnection(const slite::TCPConnectionPtr& conn);
    void onMessage(const slite::TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const slite::TCPConnectionPtr& conn);
    void onUnknownMessage(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const slite::TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onValidateRequest(const slite::TCPConnectionPtr& conn, const IMValiReqPtr& message, int64_t receiveTime);

    void onClientDepartmentRequest(const slite::TCPConnectionPtr& conn, const DepartmentReqPtr& message, int64_t receiveTime);
    void onClientAllUserRequest(const slite::TCPConnectionPtr& conn, const AllUserReqPtr& message, int64_t receiveTime);
    void onRecentContactSessionRequest(const slite::TCPConnectionPtr& conn, const RecentContactSessionReqPtr& message, int64_t receiveTime);

    void onNormalGroupListRequest(const slite::TCPConnectionPtr& conn, const NormalGroupListReqPtr& message, int64_t receiveTime);

    void onMsgData(const slite::TCPConnectionPtr& conn, const MsgDataPtr& message, int64_t receiveTime);
    void onMsgDataReadAck(const slite::TCPConnectionPtr& conn, const MsgDataReadAckPtr& message, int64_t receiveTime);
    void onUnreadMsgCntRequest(const slite::TCPConnectionPtr& conn, const UnreadMsgCntReqPtr& message, int64_t receiveTime);
    void onGetMsgListRequest(const slite::TCPConnectionPtr& conn, const GetMsgListReqPtr& message, int64_t receiveTime);
    void onGetDeviceTokenReq(const slite::TCPConnectionPtr& conn, const GetDeviceTokenReqPtr& message, int64_t receiveTime);

    bool doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo& user);

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;

    slite::TCPServer server_;
    slite::EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    slite::ProtobufCodec codec_;
    slite::ThreadPool threadPool_;
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
    std::unique_ptr<DepartmentModel> departModel_;
    std::unique_ptr<UserModel> userModel_;
    std::set<slite::TCPConnectionPtr> clientConns_;
    std::map<string, std::list<uint32_t> > hmLimits_;
    std::unique_ptr<SyncCenter> syncCenter;
};

} // namespace IM