#include "RouteServer.h"

#include "base/public_define.h"
#include "slite/Logger.h"
#include "pbs/IM.BaseDefine.pb.h"

#include <sys/time.h>
#include <random>

using namespace IM;
using namespace slite;
using namespace std::placeholders;

RouteServer::RouteServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "RouteServer"),
    loop_(loop),
    dispatcher_(std::bind(&RouteServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    server_.setConnectionCallback(
        std::bind(&RouteServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&RouteServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&RouteServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&RouteServer::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMOnlineUserInfo>(
        std::bind(&RouteServer::onOnlineUserInfo, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMUserStatusUpdate>(
        std::bind(&RouteServer::onUserStatusUpdate, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMRoleSet>(
        std::bind(&RouteServer::onRoleSet, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMUsersStatReq>(
        std::bind(&RouteServer::onUsersStatusRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMServerKickUser>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMMsgData>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::SwitchService::IMP2PCmdMsg>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMMsgDataReadNotify>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Group::IMGroupChangeMemberNotify>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::File::IMFileNotify>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMRemoveSessionNotify>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMAvatarChangedNotify>(
        std::bind(&RouteServer::onDefaultMessage, this, _1, _2, _3)); // TODO

    loop_->runEvery(1.0, std::bind(&RouteServer::onTimer, this));
}

RouteServer::~RouteServer()
{
}

void RouteServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (const auto& conn : clientConns_) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to MsgServer timeout";
            conn->forceClose();
        }
    }
}

void RouteServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);
        clientConns_.insert(conn);
    } else {
        clientConns_.erase(conn);
    }
}

void RouteServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
    codec_.onMessage(conn, buffer, receiveTime);
}

void RouteServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void RouteServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_ERROR << "onUnknownMessage: " << message->GetTypeName();
    //conn->shutdown();
}

void RouteServer::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    //LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void RouteServer::onDefaultMessage(const slite::TCPConnectionPtr& conn, 
                                    const MessagePtr& message, 
                                    int64_t receiveTime)
{
    broadcastMsg(*message.get(), conn);
}

UserManager* RouteServer::getUserInfo(uint32_t userId)
{
    UserManager* user = nullptr;
    UserManagerMap::iterator it = userManagerMap_.find(userId);
    if (it != userManagerMap_.end()) {
        user = it->second;
    }
    
    return user;
}

// 在线状态更新请求
void RouteServer::onOnlineUserInfo(const TCPConnectionPtr& conn, 
                                const OnlineUserInfoPtr& message, 
                                int64_t receiveTime)
{
	uint32_t userCount = message->user_stat_list_size();

	LOG_INFO << "onOnlineUserInfo, user_cnt = " << userCount;

	for (uint32_t i = 0; i < userCount; i++) {
        IM::BaseDefine::ServerUserStat serverUserStat = message->user_stat_list(i);
		updateUserStatus(conn, serverUserStat.user_id(), serverUserStat.status(), serverUserStat.client_type());
	}
}

void RouteServer::onRoleSet(const TCPConnectionPtr& conn, const 
                            RoleSetPtr& message, 
                            int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
	uint32_t master = message->master();

	LOG_INFO << "onRoleSet, master=" << master << ", handle= " << conn->name();
	if (master == 1) {
		context->master = true;
	} else {
		context->master = false;
	}
}

// 更新用户上下线状态信息
void RouteServer::onUserStatusUpdate(const TCPConnectionPtr& conn, 
                                const UserStatusUpdatePtr& message, 
                                int64_t receiveTime)
{
	uint32_t userStatus = message->user_status();
	uint32_t userId = message->user_id();
    uint32_t clientType = message->client_type();
	LOG_ERROR << "onUserStatusUpdate, status=" << userStatus << ", uid=" 
        << userId << ", client_type=" << clientType;

	updateUserStatus(conn, userId, userStatus, clientType);
    
    //用于通知客户端,同一用户在pc端的登录情况
    UserManager* user = getUserInfo(userId);
    if (user) {
        IM::Server::IMServerPCLoginStatusNotify msg;
        msg.set_user_id(userId);
        if (userStatus == IM::BaseDefine::USER_STATUS_OFFLINE) {
            msg.set_login_status(IM_PC_LOGIN_STATUS_OFF);
        } else {
            msg.set_login_status(IM_PC_LOGIN_STATUS_ON);
        }
        
        if (userStatus == IM::BaseDefine::USER_STATUS_OFFLINE) {
            //pc端下线且无pc端存在，则给msg_server发送一个通知
            if (CHECK_CLIENT_TYPE_PC(clientType) && !user->isPCClientLogin()) {
                broadcastMsg(msg);
            }
        } else{
            //只要pc端在线，则不管上线的是pc还是移动端，都通知msg_server
            if (user->isPCClientLogin()){
                broadcastMsg(msg);
            }
        }
    }
    
    //状态更新的是pc client端，则通知给所有其他人
    if (CHECK_CLIENT_TYPE_PC(clientType)) {
        IM::Buddy::IMUserStatNotify msg2;
        IM::BaseDefine::UserStat* user_stat = msg2.mutable_user_stat();
        user_stat->set_user_id(userId);
        user_stat->set_status((IM::BaseDefine::UserStatType)userStatus);
        //用户存在
        if (user) {
            //如果是pc客户端离线，但是仍然存在pc客户端，则不发送离线通知
            //此种情况一般是pc客户端多点登录时引起
            if (IM::BaseDefine::USER_STATUS_OFFLINE == userStatus && user->isPCClientLogin()) {
                return;
            } else {
                broadcastMsg(msg2);
            }
        } else {//该用户不存在了，则表示是离线状态
            broadcastMsg(msg2);
        }
    }
}

void RouteServer::onUsersStatusRequest(const TCPConnectionPtr& conn, 
                                        const UsersStatReqPtr& message, 
                                        int64_t receiveTime)
{
	uint32_t requestId = message->user_id();
	uint32_t queryCount = message->user_id_list_size();
	LOG_INFO << "onUsersStatusRequest, reqId=" << requestId << ", queryCount=" << queryCount;

    IM::Buddy::IMUsersStatRsp resp;
    resp.set_user_id(requestId);
    resp.set_attach_data(message->attach_data());
    for (uint32_t i = 0; i < queryCount; i++) {
        IM::BaseDefine::UserStat* userStat = resp.add_user_stat_list();
        uint32_t userId = message->user_id_list(i);
        userStat->set_user_id(userId);
        UserManager* user = getUserInfo(userId);
        if (user) {
            userStat->set_status(static_cast<::IM::BaseDefine::UserStatType>(user->getStatus()));
        } else {
            userStat->set_status(IM::BaseDefine::USER_STATUS_OFFLINE) ;
		}
	}

	// send back query user status
    codec_.send(conn, resp);
}


// update user status info, the logic seems complex
void RouteServer::updateUserStatus(const TCPConnectionPtr& conn, 
                                    uint32_t userId, 
                                    uint32_t status, 
                                    uint32_t clientType)
{
    UserManager* user = getUserInfo(userId);
    if (user) {
        // current user is already in this routeServer
        if (user->findRouteConn(conn)) {
            if (status == IM::BaseDefine::USER_STATUS_OFFLINE) {
                user->removeClientType(clientType);
                // no more user login
                if (user->isMsgConnNULL()) {
                    // no more client conn
                    user->removeRouteConn(conn);
                    if (user->getRouteConnCount() == 0) {
                        delete user;
                        user = nullptr;
                        userManagerMap_.erase(userId);
                    }
                }
            } else {
                user->addClientType(clientType);
            }
        // new client conn of current user
        } else {
            if (status != IM::BaseDefine::USER_STATUS_OFFLINE) {
                user->addRouteConn(conn);
                user->addClientType(clientType);
            }
        }
    } else {
        // new user
        if (status != IM::BaseDefine::USER_STATUS_OFFLINE) {
            UserManager* newUser = new UserManager();
            if (newUser != nullptr) {
                newUser->addRouteConn(conn);
                newUser->addClientType(clientType);
                userManagerMap_.insert(make_pair(userId, newUser));
            } else {
                LOG_ERROR << "new UserInfo failed. ";
            }
        }
    }
}

void RouteServer::broadcastMsg(const google::protobuf::Message& message, const TCPConnectionPtr& fromConn)
{
	for (const auto& conn : clientConns_) {
		if (conn != fromConn) {
			codec_.send(conn, message);
		}
	}   
}

int main(int argc, char* argv[])
{
    Logger::setLogLevel(Logger::DEBUG);
    EventLoop loop;
    RouteServer routeServer("0.0.0.0", 10004, &loop);
    routeServer.start();
    loop.loop();
}