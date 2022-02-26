#include "slite/Logger.h"
#include "ImUser.h"

#include <sys/time.h>

#include "MsgServer.h"

using namespace slite;
using namespace std::placeholders;

std::set<TCPConnectionPtr> g_clientConns;
std::set<TCPConnectionPtr> g_loginConns;
std::set<TCPConnectionPtr> g_dbProxyConns;
std::set<TCPConnectionPtr> g_routeConns;
std::set<TCPConnectionPtr> g_fileConns;

MsgServer::MsgServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "MsgServer"),
    loop_(loop),
    dispatcher_(std::bind(&MsgServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
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

    for (auto clientConn : g_clientConns) {
        Context* context = std::any_cast<Context*>(clientConn->getContext());
        
        if (currTick > context->lastRecvTick + 30000) {
            LOG_ERROR << "client timeout";
            clientConn->shutdown();
        }
    }
}

void MsgServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        g_clientConns.insert(conn);
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);
    } else {
        Context* context = std::any_cast<Context*>(conn->getContext());
        ImUser* user = ImUserManager::getInstance()->getImUserById(context->userId);
        if (user) {
            user->delMsgConn(conn->name());
            user->delUnValidateMsgConn(conn);
            
            IM::Server::IMUserCntUpdate msg;
            msg.set_user_action(USER_CNT_DEC);
            msg.set_user_id(context->clientType);

            for (auto loginConn: g_loginConns) {
                codec_.send(loginConn, msg);
            }
            
            IM::Server::IMUserStatusUpdate msg2;
            msg2.set_user_status(::IM::BaseDefine::USER_STATUS_OFFLINE);
            msg2.set_user_id(context->userId);
            msg2.set_client_type(static_cast<::IM::BaseDefine::ClientType>(context->clientType));

            for (auto routeConn: g_routeConns) {
                codec_.send(routeConn, msg);
            }
        }
        g_clientConns.erase(conn);
    }
}

void MsgServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    codec_.onMessage2(conn, buffer, receiveTime);
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
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
    //conn->shutdown();
}

void MsgServer::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
    codec_.send2(conn, *message.get());
}

void MsgServer::onLoginRequest(const TCPConnectionPtr& conn, 
                        const LoginReqPtr& message, 
                        int64_t receiveTime)
{
    LOG_INFO << "onLoginReq: username=" << message->user_name() << ", status=" << message->client_type();
    Context* context = std::any_cast<Context*>(conn->getContext());
    if (!context->loginName.empty()) {
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
    } //else if (g_routeConns.empty()) {
    //    result = IM::BaseDefine::REFUSE_REASON_NO_ROUTE_SERVER;
    //    resultStr = "服务器异常";
    //}

    if (result) {
        IM::Login::IMLoginRes msg;
        msg.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg.set_result_code(static_cast<IM::BaseDefine::ResultType>(result));
        msg.set_result_string(resultStr);
        LOG_DEBUG << " " << msg.server_time() << " " << msg.result_code() << " " << msg.result_string();
        codec_.send2(conn, msg);
        conn->shutdown();
        return;
    }

    context->loginName = message->user_name();  
    std::string password = message->password();
    uint32_t onlineStatus = message->online_status();

    if (onlineStatus < IM::BaseDefine::USER_STATUS_ONLINE || onlineStatus > IM::BaseDefine::USER_STATUS_LEAVE) {
        LOG_WARN << "HandleLoginReq, online status wrong: " << onlineStatus;
        onlineStatus = IM::BaseDefine::USER_STATUS_ONLINE;
    }
    context->clientVersion = message->client_version();
    context->clientType = message->client_type();
    context->onlineStatus = onlineStatus;
    ImUser* imUser = ImUserManager::getInstance()->getImUserByLoginName(context->loginName);
    // 只允许一个user存在，允许多个端同时登陆
    if (!imUser) {
        imUser = new ImUser(context->loginName, codec_);
        ImUserManager::getInstance()->addImUserByLoginName(context->loginName, imUser);
    }
    imUser->addUnValidateMsgConn(conn);

    TCPConnectionPtr dbProxyConn = *g_dbProxyConns.begin();
    IM::Server::IMValidateReq msg2;
    msg2.set_user_name(message->user_name());
    msg2.set_password(password);
    msg2.set_attach_data(conn->name().data(), conn->name().size());
    codec_.send(dbProxyConn, msg2);
}

void MsgServer::onLoginOutRequest(const TCPConnectionPtr& conn, 
                                const LogoutReqPtr& message, 
                                int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    LOG_INFO << "HandleLoginOutRequest, user_id=" 
        << context->userId << ", client_type= " << context->clientType;
    if (g_dbProxyConns.empty()) {
        IM::Login::IMLogoutRsp msg;
        msg.set_result_code(0);
        codec_.send2(conn, msg);
    }
    TCPConnectionPtr dbProxyConn = *g_dbProxyConns.begin();
    IM::Login::IMDeviceTokenReq msg2;
    msg2.set_user_id(context->userId);
    msg2.set_device_token("");
    codec_.send(conn, msg2);
}

void MsgServer::onClientDepartmentRequest(const TCPConnectionPtr& conn, 
                                        const DepartmentReqPtr& message, 
                                        int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    if (!g_dbProxyConns.empty()) {
        TCPConnectionPtr dbProxyConn = *g_dbProxyConns.begin();
        message->set_user_id(context->userId);
        message->set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbProxyConn, *message.get());
    }
}

void MsgServer::onRecentContactSessionRequest(const TCPConnectionPtr& conn, 
                                            const RecentContactSessionReqPtr& message, 
                                            int64_t receiveTime)
{
    if (g_dbProxyConns.empty()) return;
    TCPConnectionPtr dbConn = *g_dbProxyConns.begin();
    Context* context = std::any_cast<Context*>(conn->getContext());

    LOG_INFO << "onRecentContactSessionRequest, user_id=" << context->userId 
        << ", latest_update_time=" << message->latest_update_time();

    message->set_user_id(context->userId);
    // 请求最近联系会话列表
    message->set_attach_data(conn->name().data(), conn->name().size());
    codec_.send(dbConn, *message.get());
}

void MsgServer::onAllUserRequest(const TCPConnectionPtr& conn, 
                                const AllUserReqPtr& message, 
                                int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    uint32_t latestUpdateTime = message->latest_update_time();
    LOG_INFO << "onClientAllUserRequest, user_id=" << context->userId 
        << ", latest_update_time=" << latestUpdateTime;
    
    if (!g_dbProxyConns.empty()) {
        TCPConnectionPtr dbConn = *g_dbProxyConns.begin();
        message->set_attach_data(conn->name().data(), conn->name().size());
        codec_.send(dbConn, *message.get());
    }
}

void MsgServer::onGroupChangeMemberResponse(const TCPConnectionPtr& conn, 
                                            const GroupChangeMemberRspPtr& message, 
                                            int64_t receiveTime)
{
    LOG_INFO << "onGroupChangeMemberResponse";
    return;
}

void MsgServer::onNormalGroupListRequest(const TCPConnectionPtr& conn, 
                                        const NormalGroupListReqPtr& message, 
                                        int64_t receiveTime)
{
    LOG_INFO << "onNormalGroupListRequest";
    return;
}

void MsgServer::onUsersStatusRequest(const TCPConnectionPtr& conn, 
                                    const UsersStatReqPtr& message, 
                                    int64_t receiveTime)
{
    LOG_INFO << "onUsersStatusRequest";
    return;
}

LoginClient::LoginClient(std::string host, uint16_t port, EventLoop* loop)
    : client_(host, port, loop, "LoginClient"),
    loop_(loop),
    dispatcher_(std::bind(&LoginClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    client_.setConnectionCallback(
        std::bind(&LoginClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&LoginClient::onMessage, this, _1, _2, _3));
    client_.setWriteCompleteCallback(
        std::bind(&LoginClient::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&LoginClient::onHeartBeat, this, _1, _2, _3));
    loop_->runEvery(1.0, std::bind(&LoginClient::onTimer, this));
}

void LoginClient::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        g_loginConns.insert(conn);
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
        g_loginConns.erase(conn);
    }
}

void LoginClient::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    codec_.onMessage(conn, buffer, receiveTime);
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void LoginClient::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void LoginClient::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto conn : g_loginConns) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to LoginServer timeout";
            conn->shutdown();
        }
    }
}

void LoginClient::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

DBProxyClient::DBProxyClient(std::string host, uint16_t port, EventLoop* loop)
    : client_(host, port, loop, "DBProxyClient"),
    loop_(loop),
    dispatcher_(std::bind(&DBProxyClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
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
        std::bind(&DBProxyClient::onValidateResponseRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMDepartmentRsp>(
        std::bind(&DBProxyClient::onClientDepartmentResponse, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Buddy::IMAllUserRsp>(
        std::bind(&DBProxyClient::onClientAllUserResponse, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&DBProxyClient::onTimer, this));
}

void DBProxyClient::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);
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
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void DBProxyClient::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void DBProxyClient::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto conn : g_dbProxyConns) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to DBProxyServer timeout";
            conn->forceClose();
        }
    }
}

void DBProxyClient::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void DBProxyClient::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    return ;
}

void DBProxyClient::onValidateResponseRequest(const TCPConnectionPtr& conn, 
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
        Context* context = std::any_cast<Context*>(msgConn->getContext());
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
        user->kickOutSameClientType(context->clientType, IM::BaseDefine::KICK_REASON_DUPLICATE_USER, msgConn);
    
        if (!g_routeConns.empty()) {
            TCPConnectionPtr routeConn = *g_routeConns.begin();
            IM::Server::IMServerKickUser msg2;
            msg2.set_user_id(userId);
            msg2.set_client_type(static_cast<IM::BaseDefine::ClientType>(context->clientType));
            msg2.set_reason(1);
            codec_.send(routeConn, msg2);
        }

        LOG_INFO << "user_name: " << loginName << ", uid: " << userId;
        context->userId = userId;
        context->isOpen = true;

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
        LOG_DEBUG << "client type " << context->clientType;
        msg2.set_client_type(static_cast<IM::BaseDefine::ClientType>(context->clientType));
        for (const auto& routeConn_: g_routeConns) {
            codec_.send(routeConn_, msg2);
        }

        user->validateMsgConn(msgConn->name(), msgConn);

        // 发送登录成功和相关消息给客户端
        IM::Login::IMLoginRes msg3;
        msg3.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg3.set_result_code(IM::BaseDefine::REFUSE_REASON_NONE);
        msg3.set_result_string(resultString);
        msg3.set_online_status(static_cast<IM::BaseDefine::UserStatType>(context->onlineStatus));
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

        codec_.send2(msgConn, msg3);
    } else {
        // 发送登录失败消息
        IM::Login::IMLoginRes msg4;
        msg4.set_server_time(static_cast<uint32_t>(time(NULL)));
        msg4.set_result_code(static_cast<IM::BaseDefine::ResultType>(result));
        msg4.set_result_string(resultString);
        codec_.send2(msgConn, msg4);
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
    LOG_INFO << "HandleDepartmentResponse, user_id=" << userId << ", latestUpdateTime=" << latestUpdateTime
        << ", deptCnt=" << deptCnt; 
    
    std::string connName = message->attach_data();
    TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, connName);
    if (msgConn) { //  && msgConn->IsOpen()
        message->clear_attach_data();
        codec_.send2(msgConn, *message.get());
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
    if (msgConn) { //&& pMsgConn->IsOpen()
        message->clear_attach_data();
        codec_.send2(msgConn, *message.get());
    }
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    MsgServer msgServer("0.0.0.0", 10002, &loop);
    LoginClient loginClient("127.0.0.1", 10001, &loop);
    DBProxyClient dbProxyClient("127.0.0.1", 10003, &loop);
    msgServer.start();
    loginClient.connect();
    dbProxyClient.connect();
    loop.loop();
}