#include "DBProxyClient.h"
#include "slite/Logger.h"
#include "ImUser.h"

#include <sys/time.h>

using namespace IM;
using namespace slite;
using namespace std::placeholders;

DBProxyClient::DBProxyClient(std::string host, uint16_t port, EventLoop* loop)
    : client_(host, port, loop, "DBProxyClient"),
    loop_(loop),
    dispatcher_(std::bind(&DBProxyClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    clientCodec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    client_.setConnectionCallback(
        std::bind(&DBProxyClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&DBProxyClient::onMessage, this, _1, _2, _3));
    client_.setWriteCompleteCallback(
        std::bind(&DBProxyClient::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&DBProxyClient::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMValidateRsp>(
        std::bind(&DBProxyClient::onValidateResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMDepartmentRsp>(
        std::bind(&DBProxyClient::onClientDepartmentResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMRecentContactSessionRsp>(
        std::bind(&DBProxyClient::onRecentContactSessionResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMAllUserRsp>(
        std::bind(&DBProxyClient::onClientAllUserResponse, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Message::IMUnreadMsgCntRsp>(
        std::bind(&DBProxyClient::onUnreadMsgCntResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMGetMsgListRsp>(
        std::bind(&DBProxyClient::onGetMsgListResponse, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Group::IMNormalGroupListRsp>(
        std::bind(&DBProxyClient::onNormalGroupListResponse, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&DBProxyClient::onTimer, this));
}

void DBProxyClient::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        LOG_INFO << "connect dbproxy server success";
        ClientConnInfo* clientInfo = new ClientConnInfo();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        clientInfo->setLastRecvTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L);
        clientInfo->setLastSendTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L);
        conn->setContext(clientInfo);
        g_dbProxyConns.insert(conn);
    } else {
        g_dbProxyConns.erase(conn);
    }
}

void DBProxyClient::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    codec_.onMessage(conn, buffer, receiveTime);
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    clientInfo->setLastRecvTick(receiveTime);
}

void DBProxyClient::onWriteComplete(const TCPConnectionPtr& conn)
{
    ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    clientInfo->setLastSendTick(tval.tv_sec * 1000L + tval.tv_usec / 1000L);
}

void DBProxyClient::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (const auto& conn : g_dbProxyConns) {
        ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(conn->getContext());

        if (currTick > clientInfo->lastSendTick() + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > clientInfo->lastRecvTick() + kTimeout) {
            LOG_ERROR << "Connect to DBProxyServer timeout";
            conn->forceClose();
        }
    }
}

void DBProxyClient::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_ERROR << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void DBProxyClient::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    // do nothing
    return ;
}

void DBProxyClient::onValidateResponse(const TCPConnectionPtr& conn, 
                                        const ValidateRspPtr& message, 
                                        int64_t receiveTime)
{
    std::string loginName = message->user_name();
    uint32_t result = message->result_code();
    std::string resultString = message->result_string();
    std::string connName = message->attach_data();

    ImUser* imUser = ImUserManager::getInstance()->getImUserByLoginName(loginName);
    TCPConnectionPtr msgConn = nullptr;
    if (!imUser) {
        LOG_ERROR << "ImUser for user_name=" << loginName << " not exist";
        return;
    } else {
        msgConn = imUser->getUnValidateMsgConn(connName);
        if (!msgConn) { //  && msgConn->IsOpen()
            LOG_ERROR << "no such conn is validated, user_name=" << loginName;
            return;
        }
    }

    if (result != 0) {
        result = IM::BaseDefine::REFUSE_REASON_DB_VALIDATE_FAILED;
    }

    if (result == 0) {
        ClientConnInfo* clientInfo = std::any_cast<ClientConnInfo*>(msgConn->getContext());
        IM::BaseDefine::UserInfo userInfo = message->user_info();
        uint32_t userId = userInfo.user_id();
        ImUser* user = ImUserManager::getInstance()->getImUserById(userId);
        if (user) {
            user->addUnValidateMsgConn(msgConn);
            imUser->delUnValidateMsgConn(msgConn);
            if (imUser->isMsgConnEmpty()) {
                ImUserManager::getInstance()->removeImUserByLoginName(loginName);
                delete imUser;
            }
        } else {
            user = imUser;
        }

        // 录入用户，并踢掉同服务器的相同客户端的相同用户
        user->setUserId(userId);
        user->setNickName(userInfo.user_nick_name());
        user->setValidated();
        ImUserManager::getInstance()->addImUserById(userId, user);
        user->kickOutSameClientType(clientInfo->clientType(), IM::BaseDefine::KICK_REASON_DUPLICATE_USER, msgConn);
    
        if (!g_routeConns.empty()) {
            TCPConnectionPtr routeConn = *g_routeConns.begin();
            IM::Server::IMServerKickUser msg2;
            msg2.set_user_id(userId);
            msg2.set_client_type(static_cast<IM::BaseDefine::ClientType>(clientInfo->clientType()));
            msg2.set_reason(1);
            codec_.send(routeConn, msg2);
        }

        LOG_INFO << "user_name: " << loginName << ", uid: " << userId;
        clientInfo->setUserId(userId);
        clientInfo->setOpen();

        // 发到所有登录服务器，更新用户数
        IM::Server::IMUserCntUpdate msg;
        msg.set_user_action(USER_CNT_INC);
        msg.set_user_id(userId);
        for (const auto& loginConn: g_loginConns) {
            codec_.send(loginConn, msg);
        }
        
        // 上线信息发到所有路由服务器
        IM::Server::IMUserStatusUpdate msg2;
        msg2.set_user_status(IM::BaseDefine::USER_STATUS_ONLINE);
        msg2.set_user_id(userId);
        LOG_DEBUG << "client type " << clientInfo->clientType();
        msg2.set_client_type(static_cast<IM::BaseDefine::ClientType>(clientInfo->clientType()));
        for (const auto& routeConn_: g_routeConns) {
            codec_.send(routeConn_, msg2);
        }

        user->validateMsgConn(msgConn->name(), msgConn);

        // 发送登录成功和相关消息给客户端
        IM::Login::IMLoginRes msg3;
        msg3.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg3.set_result_code(IM::BaseDefine::REFUSE_REASON_NONE);
        msg3.set_result_string(resultString);
        msg3.set_online_status(static_cast<IM::BaseDefine::UserStatType>(clientInfo->onlineStatus()));
        IM::BaseDefine::UserInfo* userInfoTmp = msg3.mutable_user_info();
        userInfoTmp->set_user_id(userInfo.user_id());
        userInfoTmp->set_user_gender(userInfo.user_gender());
        userInfoTmp->set_user_nick_name(userInfo.user_nick_name());
        userInfoTmp->set_avatar_url(userInfo.avatar_url());
        userInfoTmp->set_sign_info(userInfo.sign_info());
        userInfoTmp->set_department_id(userInfo.department_id());
        userInfoTmp->set_email(userInfo.email());
        userInfoTmp->set_user_real_name(userInfo.user_real_name());
        userInfoTmp->set_user_tel(userInfo.user_tel());
        userInfoTmp->set_user_domain(userInfo.user_domain());
        userInfoTmp->set_status(userInfo.status());

        clientCodec_.send(msgConn, msg3);
    } else {
        // 发送登录失败消息
        IM::Login::IMLoginRes msg4;
        msg4.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg4.set_result_code(static_cast<IM::BaseDefine::ResultType>(result));
        msg4.set_result_string(resultString);
        clientCodec_.send(msgConn, msg4);
        msgConn->shutdown();
    }
}

void DBProxyClient::onClientDepartmentResponse(const TCPConnectionPtr& conn, 
                                            const DepartmentRspPtr& message, 
                                            int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t latestUpdateTime = message->latest_update_time();
    uint32_t deptCnt = message->dept_list_size();
    LOG_INFO << "onClientDepartmentResponse, user_id=" << userId << ", latestUpdateTime=" << latestUpdateTime
        << ", deptCnt=" << deptCnt; 
    
    std::string connName = message->attach_data();
    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn && msgConn->connected()) {
        message->clear_attach_data();
        clientCodec_.send(msgConn, *message.get());
    }
}

void DBProxyClient::onClientAllUserResponse(const TCPConnectionPtr& conn, 
                                            const AllUserRspPtr& message, 
                                            int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t latestUpdateTime = message->latest_update_time();
    uint32_t userCnt = message->user_list_size();
    std::string connName = message->attach_data();
    
    LOG_INFO << "onClientAllUserResponse, userId=" << userId << ", latest_update_time=" 
        << latestUpdateTime << ", user_cnt=" << userCnt;
    
    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn && msgConn->connected()) {
        message->clear_attach_data();
        clientCodec_.send(msgConn, *message.get());
    }
}

void DBProxyClient::onRecentContactSessionResponse(const TCPConnectionPtr& conn, 
                                    const RecentContactSessionRspPtr& message, 
                                    int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t sessionCnt = message->contact_session_list_size();
    std::string connName = message->attach_data();
    
    LOG_INFO << "onRecentContactSessionResponse, userId=" << userId << ", session_cnt=" << sessionCnt;
    
    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn && msgConn->connected()) {
        message->clear_attach_data();
        clientCodec_.send(msgConn, *message.get());
    }
}

void DBProxyClient::onNormalGroupListResponse(const TCPConnectionPtr& conn, 
                                            const NormalGroupListRspPtr& message, 
                                            int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t groupCnt = message->group_version_list_size();
    std::string connName = message->attach_data();

    LOG_INFO << "onNormalGroupListResponse, user_id=" 
        << userId << ", groupCnt=" << groupCnt;

    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn && msgConn->connected()) {
        message->clear_attach_data();
        clientCodec_.send(msgConn, *message.get());
    }
}

void DBProxyClient::onUnreadMsgCntResponse(const TCPConnectionPtr& conn, 
                                            const UnreadMsgCntRspPtr& message, 
                                            int64_t receiveTime)
{
	uint32_t userId = message->user_id();
    uint32_t totalCnt = message->total_cnt();
	uint32_t userUnreadCnt = message->unreadinfo_list_size();
	std::string connName = message->attach_data();
	
	LOG_INFO << "onUnreadMsgCntResponse, userId=" << userId
         << ", totalCnt=" << totalCnt << ", userUnreadCnt=" << userUnreadCnt;

    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
	if (msgConn && msgConn->connected()) {
        message->clear_attach_data();
        clientCodec_.send(msgConn, *message.get());
	}
}

void DBProxyClient::onGetMsgListResponse(const TCPConnectionPtr& conn, 
                                        const GetMsgListRspPtr& message, 
                                        int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t sessionType = message->session_type();
    uint32_t sessionId = message->session_id();
    uint32_t msgCnt = message->msg_list_size();
    uint32_t msgIdBegin = message->msg_id_begin();
    std::string connName = message->attach_data();
    
    LOG_ERROR << "onGetMsgListResponse, userId= " << userId << ", sessionType=" 
        << sessionType << ", oppositeUserId=" << sessionId << ", msgIdBegin="
        << msgIdBegin << ", cnt=" << msgCnt;
    
    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn && msgConn->connected()) {
        clientCodec_.send(msgConn, *message.get());
    }
}