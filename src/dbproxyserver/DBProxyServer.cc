#include "DBProxyServer.h"
#include "slite/Logger.h"

#include "models/SessionModel.h"
#include "models/GroupModel.h"
#include "models/MessageModel.h"
#include "models/GroupMessageModel.h"

#include "EncDec.h"

#include <sys/time.h>

using namespace slite;
using namespace IM;
using namespace std::placeholders;

DBProxyServer::DBProxyServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "MsgServer"),
    loop_(loop),
    dispatcher_(std::bind(&DBProxyServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    threadPool_("DBThreadPool"),
    dbPool_(new DBPool("IMDBPool", "127.0.0.1", 3306, "root", "jk111111", "teamtalk", 5)),
    cachePool_(new CachePool("IMCachePool", "127.0.0.1", 6379, 0, "", 4)),
    departModel_(new DepartmentModel(dbPool_)),
    userModel_(new UserModel(dbPool_)),
    syncCenter(new SyncCenter(cachePool_, dbPool_))
{
    server_.setConnectionCallback(
        std::bind(&DBProxyServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&DBProxyServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&DBProxyServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&DBProxyServer::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMValidateReq>(
        std::bind(&DBProxyServer::onValidateRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMDepartmentReq>(
        std::bind(&DBProxyServer::onClientDepartmentRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMAllUserReq>(
        std::bind(&DBProxyServer::onClientAllUserRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMRecentContactSessionReq>(
        std::bind(&DBProxyServer::onRecentContactSessionRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Group::IMNormalGroupListReq>(
        std::bind(&DBProxyServer::onNormalGroupListRequest, this, _1, _2, _3));

    dispatcher_.registerMessageCallback<IM::Message::IMUnreadMsgCntReq>(
        std::bind(&DBProxyServer::onUnreadMsgCntRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Message::IMGetMsgListReq>(
        std::bind(&DBProxyServer::onGetMsgListRequest, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&DBProxyServer::onTimer, this));
    threadPool_.setMaxQueueSize(10);
    syncCenter->startSync();
    syncCenter->init();
}

DBProxyServer::~DBProxyServer()
{
    threadPool_.stop();
    syncCenter->stopSync();
}

void DBProxyServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto conn : clientConns_) {
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

void DBProxyServer::onConnection(const TCPConnectionPtr& conn)
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

void DBProxyServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
    std::string buf;
    buf.swap(buffer);
    threadPool_.run(
        std::bind(&ProtobufCodec::onMessage, &codec_, conn, buf, receiveTime));
}

void DBProxyServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void DBProxyServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void DBProxyServer::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    //LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

bool DBProxyServer::doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo& user)
{
    bool ret = false;
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT * FROM IMUser WHERE name='" + strName + "' AND status=0";
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            string strResult, strSalt;
            uint32_t nId, nGender, nDeptId, nStatus;
            string strNick, strAvatar, strEmail, strRealName, strTel, strDomain,strSignInfo;
            while (resultSet->next()) {
                nId = resultSet->getInt("id");
                strResult = resultSet->getString("password");
                strSalt = resultSet->getString("salt");
                
                strNick = resultSet->getString("nick");
                nGender = resultSet->getInt("sex");
                strRealName = resultSet->getString("name");
                strDomain = resultSet->getString("domain");
                strTel = resultSet->getString("phone");
                strEmail = resultSet->getString("email");
                strAvatar = resultSet->getString("avatar");
                nDeptId = resultSet->getInt("departId");
                nStatus = resultSet->getInt("status");
                strSignInfo = resultSet->getString("sign_info");
            }

            string strInPass = strPass + strSalt;
            char szMd5[33];
            Md5::MD5_Calculate(strInPass.c_str(), static_cast<u_int32_t>(strInPass.length()), szMd5);
            string strOutPass(szMd5);
            if (strOutPass == strResult) {
                ret = true;
                user.set_user_id(nId);
                user.set_user_nick_name(strNick);
                user.set_user_gender(nGender);
                user.set_user_real_name(strRealName);
                user.set_user_domain(strDomain);
                user.set_user_tel(strTel);
                user.set_email(strEmail);
                user.set_avatar_url(strAvatar);
                user.set_department_id(nDeptId);
                user.set_status(nStatus);
  	            user.set_sign_info(strSignInfo);
            }
            delete resultSet;
        }
        dbPool_->relDBConn(dbConn);
    }
    return ret;
}

void DBProxyServer::onValidateRequest(const TCPConnectionPtr& conn, 
                        const IMValiReqPtr& message, 
                        int64_t receiveTime)
{
    IM::Server::IMValidateRsp resp;
    std::string strDomain = message->user_name();
    std::string strPass = message->password();

    resp.set_user_name(strDomain);
    resp.set_attach_data(message->attach_data());

    do {
        // lock
        std::list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        uint32_t tmNow = static_cast<uint32_t>(time(NULL));

        auto itTime = lsErrorTime.begin();
        for (; itTime != lsErrorTime.end(); ++itTime) {
            if (tmNow - *itTime > 30 * 60) {
                break;
            }
        }
        if (itTime != lsErrorTime.end()) {
            lsErrorTime.erase(itTime);
        }

        if (lsErrorTime.size() > 10) {
            itTime = lsErrorTime.begin();
            if (tmNow - *itTime <= 30 * 60) {
                resp.set_result_code(6);
                resp.set_result_string("用户名/密码错误次数太多");
                codec_.send(conn, resp);
                return;
            }
        }
    } while (false);

    LOG_INFO << "onValidateRequest, " << strDomain << " request login";

    IM::BaseDefine::UserInfo user;
    if (doLogin(strDomain, strPass, user)) {
        IM::BaseDefine::UserInfo* pUser = resp.mutable_user_info();
        pUser->set_user_id(user.user_id());
        pUser->set_user_gender(user.user_gender());
        pUser->set_department_id(user.department_id());
        pUser->set_user_nick_name(user.user_nick_name());
        pUser->set_user_domain(user.user_domain());
        pUser->set_avatar_url(user.avatar_url());
        
        pUser->set_email(user.email());
        pUser->set_user_tel(user.user_tel());
        pUser->set_user_real_name(user.user_real_name());
        pUser->set_status(0);

        pUser->set_sign_info(user.sign_info());
        
        resp.set_result_code(0);
        resp.set_result_string("成功");
        
        //如果登陆成功，则清除错误尝试限制
        // lock
        list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        lsErrorTime.clear();
    } else {
        //密码错误，记录一次登陆失败
        uint32_t tmCurrent =  static_cast<uint32_t>(time(NULL));
        // lock
        list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        lsErrorTime.push_front(tmCurrent);
        
        LOG_WARN << "get result false";
        resp.set_result_code(1);
        resp.set_result_string("用户名/密码错误");
    }

    codec_.send(conn, resp);
}

void DBProxyServer::onClientDepartmentRequest(const TCPConnectionPtr& conn, 
                                            const DepartmentReqPtr& message, 
                                            int64_t receiveTime)
{
    IM::Buddy::IMDepartmentRsp resp;
    uint32_t userId = message->user_id();
    uint32_t lastUpdate = message->latest_update_time();
    std::list<uint32_t> lsChangeIds;
    departModel_->getChgedDeptId(lastUpdate, lsChangeIds);
    std::list<IM::BaseDefine::DepartInfo> lsDeparts;
    departModel_->getDepts(lsChangeIds, lsDeparts);

    resp.set_user_id(userId);
    resp.set_latest_update_time(lastUpdate);
    for (auto lsDepart : lsDeparts) {
        IM::BaseDefine::DepartInfo* pDeptInfo = resp.add_dept_list();
        pDeptInfo->set_dept_id(lsDepart.dept_id());
        pDeptInfo->set_priority(lsDepart.priority());
        pDeptInfo->set_dept_name(lsDepart.dept_name());
        pDeptInfo->set_parent_dept_id(lsDepart.parent_dept_id());
        pDeptInfo->set_dept_status(lsDepart.dept_status());
    }
    LOG_INFO << "onClientDepartmentRequest, userId=" << userId << ", lastUpdate=" 
        << lastUpdate << ", cnt=" << lsDeparts.size();
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onClientAllUserRequest(const TCPConnectionPtr& conn, 
                                        const AllUserReqPtr& message, 
                                        int64_t receiveTime)
{
    IM::Buddy::IMAllUserRsp resp;
    uint32_t reqId = message->user_id();
    uint32_t lastTime = message->latest_update_time();
    uint32_t lastUpdate = syncCenter->getLastUpdate();
    
    list<IM::BaseDefine::UserInfo> lsUsers;
    if (lastUpdate > lastTime) {
        list<uint32_t> lsIds;
        userModel_->getChangedId(lastTime, lsIds);
        userModel_->getUsers(lsIds, lsUsers);
    }
    resp.set_user_id(reqId);
    resp.set_latest_update_time(lastTime);
    for (auto lsUser : lsUsers) {
        IM::BaseDefine::UserInfo* pUser = resp.add_user_list();
        pUser->set_user_id(lsUser.user_id());
        pUser->set_user_gender(lsUser.user_gender());
        pUser->set_user_nick_name(lsUser.user_nick_name());
        pUser->set_avatar_url(lsUser.avatar_url());
        pUser->set_sign_info(lsUser.sign_info());
        pUser->set_department_id(lsUser.department_id());
        pUser->set_email(lsUser.email());
        pUser->set_user_real_name(lsUser.user_real_name());
        pUser->set_user_tel(lsUser.user_tel());
        pUser->set_user_domain(lsUser.user_domain());
        pUser->set_status(lsUser.status());
    }

    LOG_INFO << "onClientAllUserRequest, userId=" << reqId << ", lastUpdate=" << lastUpdate 
        << ", last_time=" << lastTime << ", userCnt=" << resp.user_list_size();
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onRecentContactSessionRequest(const slite::TCPConnectionPtr& conn, 
                                                const RecentContactSessionReqPtr& message, 
                                                int64_t receiveTime)
{
    IM::Buddy::IMRecentContactSessionRsp resp;

    uint32_t userId = message->user_id();
    uint32_t lastTime = message->latest_update_time();
    
    //获取最近联系人列表
    list<IM::BaseDefine::ContactSessionInfo> contactList;
    SessionModel sessionModel(dbPool_, cachePool_);
    sessionModel.getRecentSession(userId, lastTime, contactList);
    resp.set_user_id(userId);
    for (const auto& contact : contactList) {
        IM::BaseDefine::ContactSessionInfo* pContact = resp.add_contact_session_list();
        //*pContact = contact;
        pContact->set_session_id(contact.session_id());
        pContact->set_session_type(contact.session_type());
        pContact->set_session_status(contact.session_status());
        pContact->set_updated_time(contact.updated_time());
        pContact->set_latest_msg_id(contact.latest_msg_id());
        pContact->set_latest_msg_data(contact.latest_msg_data());
        pContact->set_latest_msg_type(contact.latest_msg_type());
        pContact->set_latest_msg_from_user_id(contact.latest_msg_from_user_id());
    }
    
    LOG_INFO << "onRecentContactSessionRequest, userId=" << userId << ", last_time=" 
        << lastTime << ", count=" << resp.contact_session_list_size();
    
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onNormalGroupListRequest(const slite::TCPConnectionPtr& conn, 
                            const NormalGroupListReqPtr& message, 
                            int64_t receiveTime)
{
    IM::Group::IMNormalGroupListRsp resp;
    GroupModel groupModel(dbPool_, cachePool_);

    uint32_t userId = message->user_id();
    list<IM::BaseDefine::GroupVersionInfo> groups;
    groupModel.getUserGroup(userId, groups, IM::BaseDefine::GROUP_TYPE_NORMAL);
    resp.set_user_id(userId);
    for(const auto& group : groups) {
        IM::BaseDefine::GroupVersionInfo* pGroupVersion = resp.add_group_version_list();
        pGroupVersion->set_group_id(group.group_id());
        pGroupVersion->set_version(group.version());
    }
    
    LOG_INFO << "onNormalGroupListRequest, userId=" << userId << ", count=" << resp.group_version_list_size();
    
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onUnreadMsgCntRequest(const TCPConnectionPtr& conn, 
                        const UnreadMsgCntReqPtr& message, 
                        int64_t receiveTime)
{
    IM::Message::IMUnreadMsgCntRsp resp;
    MessageModel messageModel(dbPool_, cachePool_);
    GroupMessageModel groupMessageModel(dbPool_, cachePool_);
    uint32_t userId = message->user_id();

    list<IM::BaseDefine::UnreadInfo> unreadCounts;
    uint32_t totalCnt = 0;
    
    messageModel.getUnreadMsgCount(userId, totalCnt, unreadCounts);
    groupMessageModel.getUnreadMsgCount(userId, totalCnt, unreadCounts);
    resp.set_user_id(userId);
    resp.set_total_cnt(totalCnt);

    for (const auto& unreadCount : unreadCounts) {
        IM::BaseDefine::UnreadInfo* pInfo = resp.add_unreadinfo_list();
        pInfo->set_session_id(unreadCount.session_id());
        pInfo->set_session_type(unreadCount.session_type());
        pInfo->set_unread_cnt(unreadCount.unread_cnt());
        pInfo->set_latest_msg_id(unreadCount.latest_msg_id());
        pInfo->set_latest_msg_data(unreadCount.latest_msg_data());
        pInfo->set_latest_msg_type(unreadCount.latest_msg_type());
        pInfo->set_latest_msg_from_user_id(unreadCount.latest_msg_from_user_id());
    }
    
    LOG_INFO << "onUnreadMsgCntRequest, userId=" << userId << ", unreadCnt=" 
        << resp.unreadinfo_list_size() << ", totalCount=" << totalCnt;
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onGetMsgListRequest(const slite::TCPConnectionPtr& conn, 
                                        const GetMsgListReqPtr& message, 
                                        int64_t receiveTime)
{
    MessageModel messageModel(dbPool_, cachePool_);
    GroupModel groupModel(dbPool_, cachePool_);
    GroupMessageModel groupMessageModel(dbPool_, cachePool_);
    uint32_t userId = message->user_id();
    uint32_t peerId = message->session_id();
    uint32_t msgId = message->msg_id_begin();
    uint32_t msgCnt = message->msg_cnt();
    IM::BaseDefine::SessionType sessionType = message->session_type();
    if (IM::BaseDefine::SessionType_IsValid(sessionType)) {
        IM::Message::IMGetMsgListRsp resp;
        list<IM::BaseDefine::MsgInfo> msgs;
        //获取个人消息
        if (sessionType == IM::BaseDefine::SESSION_TYPE_SINGLE) {
            messageModel.getMessage(userId, peerId, msgId, msgCnt, msgs);
        //获取群消息
        } else if (sessionType == IM::BaseDefine::SESSION_TYPE_GROUP) {
            if (groupModel.isInGroup(userId, peerId)) {
                groupMessageModel.getMessage(userId, peerId, msgId, msgCnt, msgs);
            }
        }
        resp.set_user_id(userId);
        resp.set_session_id(peerId);
        resp.set_msg_id_begin(msgId);
        resp.set_session_type(sessionType);
        for (const auto& msg: msgs) {
            IM::BaseDefine::MsgInfo* pMsg = resp.add_msg_list();
            pMsg->set_msg_id(msg.msg_id());
            pMsg->set_from_session_id(msg.from_session_id());
            pMsg->set_create_time(msg.create_time());
            pMsg->set_msg_type(msg.msg_type());
            pMsg->set_msg_data(msg.msg_data());
        }
        LOG_INFO << "onGetMsgListRequest, userId=" << userId << ", peerId=" << peerId 
            << ", msgId=" << msgId  << ", msgCnt=" << msgCnt << ", count=" << resp.msg_list_size();
        resp.set_attach_data(message->attach_data());
        codec_.send(conn, resp);
    } else {
        LOG_ERROR << "invalid sessionType. userId=" << userId << ", peerId=" << peerId 
            << ", msgId="<< msgId << ", msgCnt=" << msgCnt << ", sessionType=" << sessionType;
    }
}

void DBProxyServer::onMsgData(const slite::TCPConnectionPtr& conn, 
                const MsgDataPtr& message, 
                int64_t receiveTime)
{
        uint32_t fromId = message->from_user_id();
        uint32_t toId = message->to_session_id();
        uint32_t createTime = message->create_time();
        IM::BaseDefine::MsgType msgType = message->msg_type();
        uint32_t msgLen = message->msg_data().length();
        
        uint32_t now = (uint32_t)time(NULL);
        if (IM::BaseDefine::MsgType_IsValid(msgType)) {
            if(msgLen != 0) {
                uint32_t nMsgId = 0;
                uint32_t nSessionId = 0;
                uint32_t nPeerSessionId = 0;

                MessageModel msgModel(dbPool_, cachePool_);
                GroupMessageModel groupMsgModel(dbPool_, cachePool_);
                if (msgType == IM::BaseDefine::MSG_TYPE_GROUP_TEXT) {
                    GroupModel* groupModel(dbPool_, cachePool_);
                    if (pGroupModel->isValidateGroupId(nToId) && pGroupModel->isInGroup(nFromId, nToId))
                    {
                        nSessionId = CSessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP, false);
                        if (INVALID_VALUE == nSessionId) {
                            nSessionId = CSessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP);
                        }
                        if(nSessionId != INVALID_VALUE)
                        {
                            nMsgId = pGroupMsgModel->getMsgId(nToId);
                            if (nMsgId != INVALID_VALUE) {
                                pGroupMsgModel->sendMessage(nFromId, nToId, nMsgType, nCreateTime, nMsgId, (string&)msg.msg_data());
                                CSessionModel::getInstance()->updateSession(nSessionId, nNow);
                            }
                        }
                    }
                    else
                    {
                        log("invalid groupId. fromId=%u, groupId=%u", nFromId, nToId);
                        delete pPduResp;
                        return;
                    }
                } else if (nMsgType == IM::BaseDefine::MSG_TYPE_GROUP_AUDIO) {
                    CGroupModel* pGroupModel = CGroupModel::getInstance();
                    if (pGroupModel->isValidateGroupId(nToId)&& pGroupModel->isInGroup(nFromId, nToId))
                    {
                        nSessionId = CSessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP, false);
                        if (INVALID_VALUE == nSessionId) {
                            nSessionId = CSessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_GROUP);
                        }
                        if(nSessionId != INVALID_VALUE)
                        {
                            nMsgId = pGroupMsgModel->getMsgId(nToId);
                            if(nMsgId != INVALID_VALUE)
                            {
                                pGroupMsgModel->sendAudioMessage(nFromId, nToId, nMsgType, nCreateTime, nMsgId, msg.msg_data().c_str(), nMsgLen);
                                CSessionModel::getInstance()->updateSession(nSessionId, nNow);
                            }
                        }
                    }
                    else
                    {
                        log("invalid groupId. fromId=%u, groupId=%u", nFromId, nToId);
                        delete pPduResp;
                        return;
                    }
                } else if(nMsgType== IM::BaseDefine::MSG_TYPE_SINGLE_TEXT) {
                    if (nFromId != nToId) {
                        nSessionId = CSessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                        if (INVALID_VALUE == nSessionId) {
                            nSessionId = CSessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                        }
                        nPeerSessionId = CSessionModel::getInstance()->getSessionId(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                        if(INVALID_VALUE ==  nPeerSessionId)
                        {
                            nSessionId = CSessionModel::getInstance()->addSession(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                        }
                        uint32_t nRelateId = CRelationModel::getInstance()->getRelationId(nFromId, nToId, true);
                        if(nSessionId != INVALID_VALUE && nRelateId != INVALID_VALUE)
                        {
                            nMsgId = pMsgModel->getMsgId(nRelateId);
                            if(nMsgId != INVALID_VALUE)
                            {
                                pMsgModel->sendMessage(nRelateId, nFromId, nToId, nMsgType, nCreateTime, nMsgId, (string&)msg.msg_data());
                                CSessionModel::getInstance()->updateSession(nSessionId, nNow);
                                CSessionModel::getInstance()->updateSession(nPeerSessionId, nNow);
                            }
                            else
                            {
                                log("msgId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                            }
                        }
                        else{
                            log("sessionId or relateId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                        }
                    }
                    else
                    {
                        log("send msg to self. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
                    }
                    
                } else if(nMsgType == IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO) {
                    
                    if(nFromId != nToId)
                    {
                        nSessionId = CSessionModel::getInstance()->getSessionId(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                        if (INVALID_VALUE == nSessionId) {
                            nSessionId = CSessionModel::getInstance()->addSession(nFromId, nToId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                        }
                        nPeerSessionId = CSessionModel::getInstance()->getSessionId(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE, false);
                        if(INVALID_VALUE ==  nPeerSessionId)
                        {
                            nSessionId = CSessionModel::getInstance()->addSession(nToId, nFromId, IM::BaseDefine::SESSION_TYPE_SINGLE);
                        }
                        uint32_t nRelateId = CRelationModel::getInstance()->getRelationId(nFromId, nToId, true);
                        if(nSessionId != INVALID_VALUE && nRelateId != INVALID_VALUE)
                        {
                            nMsgId = pMsgModel->getMsgId(nRelateId);
                            if(nMsgId != INVALID_VALUE) {
                                pMsgModel->sendAudioMessage(nRelateId, nFromId, nToId, nMsgType, nCreateTime, nMsgId, msg.msg_data().c_str(), nMsgLen);
                                CSessionModel::getInstance()->updateSession(nSessionId, nNow);
                                CSessionModel::getInstance()->updateSession(nPeerSessionId, nNow);
                            }
                            else {
                                log("msgId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                            }
                        }
                        else {
                            log("sessionId or relateId is invalid. fromId=%u, toId=%u, nRelateId=%u, nSessionId=%u, nMsgType=%u", nFromId, nToId, nRelateId, nSessionId, nMsgType);
                        }
                    }
                    else
                    {
                        log("send msg to self. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
                    }
                }

                log("fromId=%u, toId=%u, type=%u, msgId=%u, sessionId=%u", nFromId, nToId, nMsgType, nMsgId, nSessionId);

                msg.set_msg_id(nMsgId);
                pPduResp->SetPBMsg(&msg);
                pPduResp->SetSeqNum(pPdu->GetSeqNum());
                pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
                pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_DATA);
                CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
            }
            else
            {
                log("msgLen error. fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
            }
        }
        else
        {
            log("invalid msgType.fromId=%u, toId=%u, msgType=%u", nFromId, nToId, nMsgType);
        }
    }
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);

    EventLoop loop;
    DBProxyServer dbProxyServer("0.0.0.0", 10003, &loop);
    dbProxyServer.start();
    loop.loop();
}