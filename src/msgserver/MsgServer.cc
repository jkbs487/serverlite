#include "MsgServer.h"

#include "slite/Logger.h"
#include "ImUser.h"
#include "ClientConnInfo.h"

#include <sys/time.h>
#include <random>

using namespace IM;
using namespace slite;
using namespace std::placeholders;

//static uint32_t g_dbServerLoginCount;

TCPConnectionPtr getRandomConn(std::set<TCPConnectionPtr> conns, size_t start, size_t end)
{
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned long> u(start, end);
    while (true && !conns.empty()) {
        auto it = conns.begin();
        if (it == conns.end()) return nullptr;
        std::advance(it, u(e));
        if (it != conns.end())
            return *it;
    }
    return nullptr;
}

TCPConnectionPtr getRandomDBProxyConnForLogin()
{
    return getRandomConn(g_dbProxyConns, 0, g_dbProxyConns.size()-1);
}

TCPConnectionPtr getRandomDBProxyConn()
{
    return getRandomConn(g_dbProxyConns, g_dbProxyConns.size()/2, g_dbProxyConns.size()-1);
}

TCPConnectionPtr getRandomRouteConn()
{
    return getRandomConn(g_routeConns, 0, g_routeConns.size()-1);
}

MsgServer::MsgServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "MsgServer"),
    loop_(loop),
    dispatcher_(std::bind(&MsgServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    clientCodec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    server_.setConnectionCallback(
        std::bind(&MsgServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&MsgServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&MsgServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&MsgServer::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Login::IMLoginReq>(
        std::bind(&MsgServer::onLoginRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Login::IMLogoutReq>(
        std::bind(&MsgServer::onLoginOutRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMDepartmentReq>(
        std::bind(&MsgServer::onClientDepartmentRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMRecentContactSessionReq>(
        std::bind(&MsgServer::onRecentContactSessionRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMAllUserReq>(
        std::bind(&MsgServer::onAllUserRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMUsersStatReq>(
        std::bind(&MsgServer::onUsersStatusRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Group::IMGroupChangeMemberRsp>(
        std::bind(&MsgServer::onGroupChangeMemberResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Group::IMNormalGroupListReq>(
        std::bind(&MsgServer::onNormalGroupListRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Message::IMMsgData>(
        std::bind(&MsgServer::onMsgData, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMUnreadMsgCntReq>(
        std::bind(&MsgServer::onUnreadMsgCntRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMGetMsgListReq>(
        std::bind(&MsgServer::onGetMsgListRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::File::IMFileHasOfflineReq>(
        std::bind(&MsgServer::onFileHasOfflineRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::SwitchService::IMP2PCmdMsg>(
        std::bind(&MsgServer::onP2PCmdMsg, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&MsgServer::onTimer, this));
}

MsgServer::~MsgServer()
{
}

void MsgServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (const auto& clientConn : g_clientConns) {
        ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(clientConn->getContext());
        
        if (currTick > clientInfo->lastRecvTick() + 30000) {
            LOG_ERROR << "client timeout";
            clientConn->shutdown();
        }
    }
}

void MsgServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        g_clientConns.insert(conn);
        ClientConnInfo* clientInfo = new ClientConnInfo();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        clientInfo->setLastRecvTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L); 
        clientInfo->setLastSendTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L); 
        conn->setContext(clientInfo);
    } else {
        ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
        ImUser* user = ImUserManager::getInstance()->getImUserById(clientInfo->userId());
        if (user) {
            user->delMsgConn(conn->name());
            user->delUnValidateMsgConn(conn);
            
            IM::Server::IMUserCntUpdate msg;
            msg.set_user_action(USER_CNT_DEC);
            msg.set_user_id(clientInfo->clientType());

            for (auto loginConn: g_loginConns) {
                codec_.send(loginConn, msg);
            }
            
            IM::Server::IMUserStatusUpdate msg2;
            msg2.set_user_status(::IM::BaseDefine::USER_STATUS_OFFLINE);
            msg2.set_user_id(clientInfo->userId());
            msg2.set_client_type(static_cast<::IM::BaseDefine::ClientType>(clientInfo->clientType()));

            for (auto routeConn: g_routeConns) {
                codec_.send(routeConn, msg2);
            }
        }
        g_clientConns.erase(conn);
    }
}

void MsgServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    clientCodec_.onMessage(conn, buffer, receiveTime);
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    clientInfo->setLastRecvTick(receiveTime);
}

void MsgServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    clientInfo->setLastSendTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L);
}

void MsgServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    //conn->shutdown();
}

void MsgServer::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    //LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    clientInfo->setLastRecvTick(receiveTime);
    clientCodec_.send(conn, *message.get());
}

void MsgServer::onLoginRequest(const TCPConnectionPtr& conn, 
                        const LoginReqPtr& message, 
                        int64_t receiveTime)
{
    LOG_INFO << "onLoginReq: username=" << message->user_name() 
        << ", status=" << message->client_type();
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    if (!clientInfo->loginName().empty()) {
        LOG_WARN << "duplicate LoginRequest in the same conn";
        return;
    }

    uint32_t result = 0;
    std::string resultStr = "";
    if (g_dbProxyConns.empty()) {
        result = IM::BaseDefine::REFUSE_REASON_NO_DB_SERVER;
        resultStr = "服务器异常";
    } else if (g_loginConns.empty()) {
        result = IM::BaseDefine::REFUSE_REASON_NO_LOGIN_SERVER;
        resultStr = "服务器异常";
    } else if (g_routeConns.empty()) {
        result = IM::BaseDefine::REFUSE_REASON_NO_ROUTE_SERVER;
        resultStr = "服务器异常";
    }

    if (result) {
        IM::Login::IMLoginRes msg;
        msg.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg.set_result_code(static_cast<IM::BaseDefine::ResultType>(result));
        msg.set_result_string(resultStr);
        LOG_DEBUG << " " << msg.server_time() << " " << msg.result_code() << " " << msg.result_string();
        clientCodec_.send(conn, msg);
        conn->shutdown();
        return;
    }

    clientInfo->setLoginName(message->user_name());  
    std::string password = message->password();
    uint32_t onlineStatus = message->online_status();

    if (onlineStatus < IM::BaseDefine::USER_STATUS_ONLINE || onlineStatus > IM::BaseDefine::USER_STATUS_LEAVE) {
        LOG_WARN << "onLoginReq, online status wrong: " << onlineStatus;
        onlineStatus = IM::BaseDefine::USER_STATUS_ONLINE;
    }
    clientInfo->setClientVersion(message->client_version());
    clientInfo->setClientType(message->client_type());
    clientInfo->setOnlineStatus(onlineStatus);
    ImUser* imUser = ImUserManager::getInstance()->getImUserByLoginName(clientInfo->loginName());
    // 只允许一个user存在，允许多个端同时登陆
    if (!imUser) {
        imUser = new ImUser(clientInfo->loginName(), clientCodec_);
        ImUserManager::getInstance()->addImUserByLoginName(clientInfo->loginName(), imUser);
    }
    imUser->addUnValidateMsgConn(conn);
    
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        IM::Server::IMValidateReq msg2;
        msg2.set_user_name(message->user_name());
        msg2.set_password(password);
        msg2.set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbProxyConn, msg2);
    }
}

void MsgServer::onLoginOutRequest(const TCPConnectionPtr& conn, 
                                const LogoutReqPtr& message, 
                                int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    LOG_INFO << "HandleLoginOutRequest, user_id=" 
        << clientInfo->userId() << ", client_type= " << clientInfo->clientType();

    TCPConnectionPtr dbProxyConn = getRandomDBProxyConnForLogin();
    if (!dbProxyConn) {
        IM::Login::IMLogoutRsp msg;
        msg.set_result_code(0);
        clientCodec_.send(conn, msg);
    } else {
        IM::Login::IMDeviceTokenReq msg2;
        msg2.set_user_id(clientInfo->userId());
        msg2.set_device_token("");
        codec_.send(conn, msg2);
    }
}

void MsgServer::onClientDepartmentRequest(const TCPConnectionPtr& conn, 
                                        const DepartmentReqPtr& message, 
                                        int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_user_id(clientInfo->userId());
        message->set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onRecentContactSessionRequest(const TCPConnectionPtr& conn, 
                                            const RecentContactSessionReqPtr& message, 
                                            int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    LOG_INFO << "onRecentContactSessionRequest, user_id=" << clientInfo->userId() 
        << ", latest_update_time=" << message->latest_update_time();
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_user_id(clientInfo->userId());
        // 请求最近联系会话列表
        message->set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onAllUserRequest(const TCPConnectionPtr& conn, 
                                const AllUserReqPtr& message, 
                                int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    uint32_t latestUpdateTime = message->latest_update_time();
    LOG_INFO << "onClientAllUserRequest, user_id=" << clientInfo->userId() 
        << ", latest_update_time=" << latestUpdateTime;
    
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onGroupChangeMemberResponse(const TCPConnectionPtr& conn, 
                                            const GroupChangeMemberRspPtr& message, 
                                            int64_t receiveTime)
{
    LOG_INFO << "onGroupChangeMemberResponse";
    return;
}

void MsgServer::onFileHasOfflineRequest(const TCPConnectionPtr& conn, 
                                        const FileHasOfflineReqPtr& message, 
                                        int64_t receiveTime)
{
LOG_INFO << "onFileHasOfflineRequest";
return;
}

void MsgServer::onNormalGroupListRequest(const TCPConnectionPtr& conn, 
                                        const NormalGroupListReqPtr& message, 
                                        int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    uint32_t userId = clientInfo->userId();
    LOG_INFO << "onClientGroupNormalRequest, user_id=" << userId;
    
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_user_id(userId);
        message->set_attach_data(conn->name());
        codec_.send(dbProxyConn, *message.get());
    } else {
        LOG_ERROR << "no db connection. ";
        IM::Group::IMNormalGroupListRsp msg2;
        message->set_user_id(userId);
        clientCodec_.send(conn, msg2);
    }
}

void MsgServer::onUsersStatusRequest(const TCPConnectionPtr& conn, 
                                    const UsersStatReqPtr& message, 
                                    int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    uint32_t userId = clientInfo->userId();
    LOG_INFO << "onUnreadMsgCntRequest, user_id=" << userId;
    
    TCPConnectionPtr routeConn = getRandomRouteConn();
    if (routeConn) {
        message->set_user_id(userId);
        message->set_attach_data(conn->name());
        codec_.send(routeConn, *message.get());
    }
}

void MsgServer::onUnreadMsgCntRequest(const TCPConnectionPtr& conn, 
                            const UnreadMsgCntReqPtr& message, 
                            int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    uint32_t userId = clientInfo->userId();
    LOG_INFO << "onUsersStatusRequest, user_id=" << userId;
    
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_user_id(userId);

        message->set_attach_data(conn->name());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onGetMsgListRequest(const TCPConnectionPtr& conn, 
                                    const GetMsgListReqPtr& message, 
                                    int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    uint32_t userId = clientInfo->userId();
    uint32_t sessionId = message->session_id();
    uint32_t msgIdBegin = message->msg_id_begin();
    uint32_t msgCnt = message->msg_cnt();
    uint32_t sessionType = message->session_type();
    LOG_INFO << "onGetMsgListRequest, userId=" << userId << ", sessionType=" << sessionType
        << ", sessionId=" << sessionId << ", msgIdBegin=" << msgIdBegin << ", msgCnt=" << msgCnt;
    
    TCPConnectionPtr dbProxyConn = getRandomDBProxyConn();
    if (dbProxyConn) {
        message->set_user_id(userId);
        message->set_attach_data(conn->name());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onP2PCmdMsg(const TCPConnectionPtr& conn, 
                            const P2PCmdMsgPtr& message, 
                            int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
	string cmdMsg = message->cmd_msg_data();
	uint32_t fromUserId = message->from_user_id();
	uint32_t toUserId = message->to_user_id();

	LOG_ERROR << "onP2PCmdMsg, " << fromUserId << "->" << toUserId << ", cmd_msg: " << cmdMsg;

    ImUser* fromImUser = ImUserManager::getInstance()->getImUserById(clientInfo->userId());
	ImUser* toImUser = ImUserManager::getInstance()->getImUserById(toUserId);
    
	if (fromImUser) {
		fromImUser->broadcastMsg(message, conn);
	}
    
	if (toImUser) {
		toImUser->broadcastMsg(message);
	}
    
	TCPConnectionPtr routeConn = getRandomRouteConn();
	if (routeConn) {
		codec_.send(routeConn, *message.get());
	}
}

void MsgServer::onMsgData(const slite::TCPConnectionPtr& conn, 
                        const MsgDataPtr& message, 
                        int64_t receiveTime)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
	if (message->msg_data().length() == 0) {
		LOG_WARN << "discard an empty message, uid=" << clientInfo->userId();
		return;
	}

	if (clientInfo->msgCntPerSec() >= MAX_MSG_CNT_PER_SECOND) {
		LOG_WARN << "!!!too much msg cnt in one second, uid=" << clientInfo->userId();
		return;
	}
    
    if (message->from_user_id() == message->to_session_id() && CHECK_MSG_TYPE_SINGLE(message->msg_type()))
    {
        LOG_WARN << "!!!from_user_id == to_user_id";
        return;
    }

	clientInfo->incrMsgCntPerSec();

	uint32_t toSessionId = message->to_session_id();
    uint32_t msgId = message->msg_id();
	uint8_t msgType = message->msg_type();
    string msgData = message->msg_data();

    // if logtoggle , SIGUSER2 pid
	LOG_INFO << "onMsgData, " << clientInfo->userId() << "->" << toSessionId 
        << ", msgType=" << msgType << ", msgId=" << msgId;

    message->set_from_user_id(clientInfo->userId());
    message->set_create_time(static_cast<uint32_t>(::time(NULL)));
    message->set_attach_data(conn->name());
	TCPConnectionPtr dbConn = getRandomDBProxyConn();
	if (dbConn) {
		codec_.send(dbConn, *message.get());
	}
}