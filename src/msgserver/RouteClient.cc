#include "RouteClient.h"
#include "slite/Logger.h"
#include "ImUser.h"

#include <sys/time.h>

using namespace IM;
using namespace slite;
using namespace std::placeholders;

RouteClient::RouteClient(std::string host, uint16_t port, EventLoop* loop)
    : client_(host, port, loop, "RouteClient"),
    loop_(loop),
    dispatcher_(std::bind(&RouteClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    clientCodec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
{
    client_.setConnectionCallback(
        std::bind(&RouteClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RouteClient::onMessage, this, _1, _2, _3));
    client_.setWriteCompleteCallback(
        std::bind(&RouteClient::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&RouteClient::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMUsersStatRsp>(
        std::bind(&RouteClient::onUsersStatusResponse, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMServerKickUser>(
        std::bind(&RouteClient::onKickUser, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMUserStatNotify>(
        std::bind(&RouteClient::onStatusNotify, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMMsgData>(
        std::bind(&RouteClient::onMsgData, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMServerPCLoginStatusNotify>(
        std::bind(&RouteClient::onPCLoginStatusNotify, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMRemoveSessionNotify>(
        std::bind(&RouteClient::onRemoveSessionNotify, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMSignInfoChangedNotify>(
        std::bind(&RouteClient::onSignInfoChangedNotify, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&RouteClient::onTimer, this));
}

void RouteClient::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        LOG_INFO << "connect to route server success";

        g_routeConns.insert(conn);
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);

       // if (g_master_rs_conn == NULL) {
       //     update_master_route_serv_conn();
       // }

        list<user_stat_t> onlineUsers;
        ImUserManager::getInstance()->getOnlineUserInfo(&onlineUsers);
        IM::Server::IMOnlineUserInfo msg;
        for (const auto& onlineUser : onlineUsers) {
            IM::BaseDefine::ServerUserStat* serverUserStat = msg.add_user_stat_list();
            serverUserStat->set_user_id(onlineUser.user_id);
            serverUserStat->set_status(static_cast<::IM::BaseDefine::UserStatType>(onlineUser.status));
            serverUserStat->set_client_type(static_cast<::IM::BaseDefine::ClientType>(onlineUser.client_type));
        }
        codec_.send(conn, msg);
    } else {
        LOG_INFO << "close from route server " << conn->name();
        g_routeConns.erase(conn);
    }
}

void RouteClient::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    codec_.onMessage(conn, buffer, receiveTime);
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

void RouteClient::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void RouteClient::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (const auto& conn : g_routeConns) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "connect to RouteServer timeout";
            // do not use shutdown，prevent can not recv FIN
            conn->forceClose();
        }
    }
}

void RouteClient::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_ERROR << "onUnknownMessage: " << message->GetTypeName();
    //conn->shutdown();
}

void RouteClient::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    // do nothing
    return ;
}

void RouteClient::onKickUser(const slite::TCPConnectionPtr& conn, 
                const ServerKickUserPtr& message, 
                int64_t receiveTime)
{
	uint32_t userId = message->user_id();
    uint32_t clientType = message->client_type();
    uint32_t reason = message->reason();
	LOG_INFO << "onKickUser, userId=" << userId << ", clientType=" 
        << clientType << ", reason= " << reason;

    ImUser* user = ImUserManager::getInstance()->getImUserById(userId);
	if (user) {
		user->kickOutSameClientType(clientType, reason);
	}
}

void RouteClient::onP2PMsg(const slite::TCPConnectionPtr& conn, 
            const P2PCmdMsgPtr& message, 
            int64_t receiveTime)
{
	uint32_t fromUserId = message->from_user_id();
	uint32_t toUserId = message->to_user_id();

	LOG_INFO << "onP2PMsg, " << fromUserId << "->" << toUserId;
    
    ImUser* fromImUser = ImUserManager::getInstance()->getImUserById(fromUserId);
	ImUser* toImUser = ImUserManager::getInstance()->getImUserById(toUserId);
    
 	if (fromImUser) {
 		fromImUser->broadcastMsg(message);
	}
    
 	if (toImUser) {
 		toImUser->broadcastMsg(message);
	}
}

void RouteClient::onStatusNotify(const slite::TCPConnectionPtr& conn, 
                    const UserStatNotifyPtr& message, 
                    int64_t receiveTime)
{
    IM::BaseDefine::UserStat userStat = message->user_stat();
	LOG_INFO << "onStatusNotify, userId=" << userStat.user_id() << ", status=" << userStat.status();

	// send friend online message to client
    ImUserManager::getInstance()->broadcastMsg(message, CLIENT_TYPE_FLAG_PC);
}

// 消息发给所有用户包括自己
void RouteClient::onMsgData(const slite::TCPConnectionPtr& conn, 
                const MsgDataPtr& message, 
                int64_t receiveTime)
{
    if (CHECK_MSG_TYPE_GROUP(message->msg_type())) {
        //
        return;
    }
	uint32_t fromUserId = message->from_user_id();
	uint32_t toUserId = message->to_session_id();
    uint32_t msgId = message->msg_id();

	LOG_INFO << "onMsgData, " << fromUserId << "->" << toUserId 
        << ", msg_id= " << msgId;
    
    ImUser* fromImUser = ImUserManager::getInstance()->getImUserById(fromUserId);
    if (fromImUser) {
        fromImUser->broadcastClientMsgData(message, msgId, nullptr, fromUserId);
    }
    
	ImUser* toImUser = ImUserManager::getInstance()->getImUserById(toUserId);
	if (toImUser) {
		toImUser->broadcastClientMsgData(message, msgId, nullptr, fromUserId);
	}
}

void RouteClient::onMsgReadNotify(const TCPConnectionPtr& conn, 
                                const MsgDataReadNotifyPtr& message, 
                                int64_t receiveTime)
{
    uint32_t reqId = message->user_id();
    uint32_t sessionId = message->session_id();
    uint32_t msgId = message->msg_id();
    uint32_t sessionType = message->session_type();
    
    LOG_INFO << "onMsgReadNotify, user_id=" << reqId << ", session_id=" 
        << sessionId << ", session_type=" << sessionType << ", msg_id=" << msgId;
    ImUser* user = ImUserManager::getInstance()->getImUserById(reqId);
    if (user) {
        user->broadcastMsg(message);
    }
}

void RouteClient::onUsersStatusResponse(const slite::TCPConnectionPtr& conn, 
                        const UsersStatRspPtr& message, 
                        int64_t receiveTime)
{
	uint32_t userId = message->user_id();
	uint32_t resultCount = message->user_stat_list_size();
	LOG_INFO << "onUsersStatusResp, userId=" << userId << ", queryCount=" << resultCount;
    
        TCPConnectionPtr msgConn = ImUserManager::getInstance()->getMsgConnByHandle(userId, message->attach_data());
        if (msgConn) {
            message->clear_attach_data();
            clientCodec_.send(msgConn, *message.get());
        }
/*
    else if (attach_data.GetType() == ATTACH_TYPE_HANDLE_AND_PDU_FOR_FILE)
    {
        IM::BaseDefine::UserStat user_stat = msg.user_stat_list(0);
        IM::Server::IMFileTransferReq msg3;
        CHECK_PB_PARSE_MSG(msg3.ParseFromArray(attach_data.GetPdu(), attach_data.GetPduLength()));
        uint32_t handle = attach_data.GetHandle();
        
        IM::BaseDefine::TransferFileType trans_mode = IM::BaseDefine::FILE_TYPE_OFFLINE;
        if (user_stat.status() == IM::BaseDefine::USER_STATUS_ONLINE)
        {
            trans_mode = IM::BaseDefine::FILE_TYPE_ONLINE;
        }
        msg3.set_trans_mode(trans_mode);
        CImPdu pdu;
        pdu.SetPBMsg(&msg3);
        pdu.SetServiceId(SID_OTHER);
        pdu.SetCommandId(CID_OTHER_FILE_TRANSFER_REQ);
        pdu.SetSeqNum(pPdu->GetSeqNum());
        CFileServConn* pConn = get_random_file_serv_conn();
        if (pConn) {
            pConn->SendPdu(&pdu);
        }
        else
        {
            log("no file server ");
            IM::File::IMFileRsp msg4;
            msg4.set_result_code(1);
            msg4.set_from_user_id(msg3.from_user_id());
            msg4.set_to_user_id(msg3.to_user_id());
            msg4.set_file_name(msg3.file_name());
            msg4.set_task_id("");
            msg4.set_trans_mode(msg3.trans_mode());
            CImPdu pdu2;
            pdu2.SetPBMsg(&msg4);
            pdu2.SetServiceId(SID_FILE);
            pdu2.SetCommandId(CID_FILE_RESPONSE);
            pdu2.SetSeqNum(pPdu->GetSeqNum());
            CMsgConn* pMsgConn = CImUserManager::GetInstance()->GetMsgConnByHandle(msg3.from_user_id(),handle);
            if (pMsgConn)
            {
                pMsgConn->SendPdu(&pdu2);
            }
        }
    }
    */
}

void RouteClient::onPCLoginStatusNotify(const slite::TCPConnectionPtr& conn, 
                                        const PCLoginStatusNotifyPtr& message, 
                                        int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t loginStatus = message->login_status();
    LOG_INFO << "onPCLoginStatusNotify, user_id=" << userId << ", login_status=" << loginStatus;
    
    ImUser* user = ImUserManager::getInstance()->getImUserById(userId);
    if (user) {
        user->setPCLoginStatus(loginStatus);
        IM::Buddy::IMPCLoginStatusNotify msg2;
        msg2.set_user_id(userId);
        if (IM_PC_LOGIN_STATUS_ON == loginStatus) {
            msg2.set_login_stat(::IM::BaseDefine::USER_STATUS_ONLINE);
        }
        else {
            msg2.set_login_stat(::IM::BaseDefine::USER_STATUS_OFFLINE);
        }
        user->broadcastMsgToMobile(message);
    }
}

void RouteClient::onRemoveSessionNotify(const slite::TCPConnectionPtr& conn, 
                                        const RemoveSessionNotifyPtr& message, 
                                        int64_t receiveTime)
{
    uint32_t userId = message->user_id();
    uint32_t sessionId = message->session_id();
    LOG_INFO << "onRemoveSessionNotify, user_id=" << userId << ", session_id=" << sessionId;
    ImUser* user = ImUserManager::getInstance()->getImUserById(userId);
    if (user) {
        user->broadcastMsg(message);
    }
}

void RouteClient::onSignInfoChangedNotify(const slite::TCPConnectionPtr& conn, 
                                        const SignInfoChangedNotifyPtr& message, 
                                        int64_t receiveTime)
{
    LOG_INFO << "onSignInfoChangedNotify, changed_user_id=" << message->changed_user_id() 
        << ", sign_info=" << message->sign_info();
    // send friend online message to client
    ImUserManager::getInstance()->broadcastMsg(message, CLIENT_TYPE_FLAG_BOTH);
}