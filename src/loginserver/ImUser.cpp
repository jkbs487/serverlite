/*
 * ImUser.cpp
 *
 *  Created on: 2014年4月16日
 *      Author: ziteng
 *  Brief:
 *  	a map from user_id to userInfo and connection list
 */
#include "Logger.h"
#include "ImUser.h"
#include "IM.Server.pb.h"
#include "IM.Login.pb.h"

using namespace ::IM::BaseDefine;

ImUser::ImUser(std::string userName, ProtobufCodec codec)
    :userId_(0),
    loginName_(userName),
    userUpdated_(false),
    pcLoginStatus_(IM::BaseDefine::USER_STATUS_OFFLINE),
    validate_(false),
    codec_(codec)
{
    //log("ImUser, userId=%u\n", user_id);
}

ImUser::~ImUser()
{
    //log("~ImUser, userId=%u\n", m_user_id);
}

TCPConnectionPtr ImUser::getUnValidateMsgConn(std::string name)
{
    for (auto conn: unvalidateConnSet_) {
        if (conn->name() == name) {
            return conn;
        }
    }

    return nullptr;
}

TCPConnectionPtr ImUser::getMsgConn(std::string name)
{
    TCPConnectionPtr conn = NULL;
    auto it = conns_.find(name);
    if (it != conns_.end()) {
        conn = it->second;
    }
    return conn;
}

void ImUser::validateMsgConn(std::string name, TCPConnectionPtr msgConn)
{
    addMsgConn(name, msgConn);
    delUnValidateMsgConn(msgConn);
}


user_conn_t ImUser::getUserConn()
{
    uint32_t connCnt = 0;
    for (auto conn: conns_) {
        if (conn.second->connected()) {
            connCnt++;
        }
    }
    
    user_conn_t userCnt = {userId_, connCnt};
    return userCnt;
}

void ImUser::broadcastMsg(MessagePtr message, TCPConnectionPtr fromConn)
{
    for (auto conn: conns_) {
        if (conn.second != fromConn) {
            codec_.send(conn.second, *message.get());
        }
    }
}

void ImUser::broadcastMsgWithOutMobile(MessagePtr message, TCPConnectionPtr fromConn)
{
    for (auto conn: conns_) {
        Context* context = std::any_cast<Context*>(conn.second->getContext());
        if (conn.second != fromConn && CHECK_CLIENT_TYPE_PC(context->clientType)) {
            codec_.send(conn.second, *message.get());
        }
    }
}

void ImUser::broadcastMsgToMobile(MessagePtr message, TCPConnectionPtr fromConn)
{
    for (auto conn: conns_) {
        Context* context = std::any_cast<Context*>(conn.second->getContext());
        if (conn.second != fromConn && CHECK_CLIENT_TYPE_PC(context->clientType)) {
            codec_.send(conn.second, *message.get());
        }
    }
}

void ImUser::broadcastClientMsgData(MessagePtr message, uint32_t msgId, TCPConnectionPtr fromConn, uint32_t fromId)
{
    for (auto conn: conns_) {
        Context* context = std::any_cast<Context*>(conn.second->getContext());
        if (conn.second != fromConn && CHECK_CLIENT_TYPE_PC(context->clientType)) {
            codec_.send(conn.second, *message.get());
            //context->sendMsg.push_back(msgId, fromId);
        }
    }
}

void ImUser::broadcastData(void *buff, uint32_t len, TCPConnectionPtr fromConn)
{
    if (!buff) return;
    for (auto conn: conns_) {
        Context* context = std::any_cast<Context*>(conn.second->getContext());
        if (conn.second != fromConn && CHECK_CLIENT_TYPE_PC(context->clientType)) {
            conn.second->send(std::string(static_cast<const char*>(buff), len));
        }
    }
}

void ImUser::handleKickUser(TCPConnectionPtr msgConn, uint32_t reason)
{
    auto it = conns_.find(msgConn->name());
    if (it != conns_.end()) {
        TCPConnectionPtr conn = it->second;
        if(conn) {
            LOG_INFO << "kick service user, user_id=" << userId_;
            IM::Login::IMKickUser msg;
            msg.set_user_id(userId_);
            msg.set_kick_reason(static_cast<::IM::BaseDefine::KickReasonType>(reason));
            codec_.send(conn, msg);
            Context* context = std::any_cast<Context*>(conn->getContext());
            context->kickOff = true;
        }
    }
}

// 只支持一个WINDOWS/MAC客户端登陆,或者一个ios/android登录
bool ImUser::kickOutSameClientType(uint32_t clientType, uint32_t reason, TCPConnectionPtr fromConn)
{

    for (auto conn: conns_) {
        Context* context = std::any_cast<Context*>(conn.second->getContext());
        //16进制位移计算
        if ((((context->clientType ^ clientType) >> 4) == 0) && (conn.second != fromConn)) {
            handleKickUser(conn.second, reason);
            break;
        }
    }
    return true;
}

uint32_t ImUser::getClientTypeFlag()
{
    uint32_t clientTypeFlag = 0x00;
    map<std::string, TCPConnectionPtr>::iterator it = conns_.begin();
    for (; it != conns_.end(); it++)
    {
        TCPConnectionPtr conn = it->second;
        Context* context = std::any_cast<Context*>(conn->getContext());
        uint32_t clientType = context->clientType;
        if (CHECK_CLIENT_TYPE_PC(clientType))
        {
            clientTypeFlag |= CLIENT_TYPE_FLAG_PC;
        }
        else if (CHECK_CLIENT_TYPE_MOBILE(clientType))
        {
            clientTypeFlag |= CLIENT_TYPE_FLAG_MOBILE;
        }
    }
    return clientTypeFlag;
}


ImUserManager::~ImUserManager()
{
    removeAll();
}

ImUserManager* ImUserManager::getInstance()
{
    static ImUserManager s_manager;
    return &s_manager;
}


ImUser* ImUserManager::getImUserByLoginName(string loginName)
{
    ImUser* pUser = NULL;
    ImUserMapByName_t::iterator it = imUsersByName_.find(loginName);
    if (it != imUsersByName_.end()) {
        pUser = it->second;
    }
    return pUser;
}

ImUser* ImUserManager::getImUserById(uint32_t userId)
{
    ImUser* pUser = NULL;
    ImUserMap_t::iterator it = imUsers_.find(userId);
    if (it != imUsers_.end()) {
        pUser = it->second;
    }
    return pUser;
}

TCPConnectionPtr ImUserManager::getMsgConnByHandle(uint32_t userId, std::string name)
{
    TCPConnectionPtr conn = nullptr;
    ImUser* pImUser = getImUserById(userId);
    if (pImUser) {
        conn = pImUser->getMsgConn(name);
    }
    return conn;
}

bool ImUserManager::addImUserByLoginName(std::string loginName, ImUser *pUser)
{
    bool bRet = false;
    if (getImUserByLoginName(loginName) == NULL) {
        imUsersByName_[loginName] = pUser;
        bRet = true;
    }
    return bRet;
}

void ImUserManager::removeImUserByLoginName(std::string loginName)
{
    imUsersByName_.erase(loginName);
}

bool ImUserManager::addImUserById(uint32_t userId, ImUser *pUser)
{
    bool bRet = false;
    if (getImUserById(userId) == NULL) {
        imUsers_[userId] = pUser;
        bRet = true;
    }
    return bRet;
}

void ImUserManager::removeImUserById(uint32_t userId)
{
    imUsers_.erase(userId);
}

void ImUserManager::removeImUser(ImUser *pUser)
{
    if (pUser != NULL) {
        removeImUserById(pUser->getUserId());
        removeImUserByLoginName(pUser->getLoginName());
        delete pUser;
        pUser = NULL;
    }
}

void ImUserManager::removeAll()
{
    for (auto imUser: imUsersByName_) {
        if (imUser.second != nullptr) {
            delete imUser.second;
            imUser.second = nullptr;
        }
    }
    imUsersByName_.clear();
    imUsers_.clear();
}

void ImUserManager::getOnlineUserInfo(list<user_stat_t>* onlineUserInfo)
{
    user_stat_t status;

    for (auto imUser: imUsers_) {
        if (imUser.second->isValidate()) {
            auto conns = imUser.second->getMsgConnMap();
            for (auto conn: conns) {
                if (conn.second->connected()) {
                    Context* context = std::any_cast<Context*>(conn.second->getContext());
                    status.user_id = imUser.second->getUserId();
                    status.client_type = context->clientType;
                    status.status = context->onlineStatus;
                    onlineUserInfo->push_back(status);
                }
            }
        }
    }
}

void ImUserManager::getUserConnCnt(list<user_conn_t>* userConnList, uint32_t& totalConnCnt)
{
    totalConnCnt = 0;
    for (auto imUser : imUsers_) {
        if (imUser.second->isValidate())
        {
            user_conn_t user_conn_cnt = imUser.second->getUserConn();
            userConnList->push_back(user_conn_cnt);
            totalConnCnt += user_conn_cnt.conn_cnt;
        }
    }
}

void ImUserManager::broadcastMsg(MessagePtr message, uint32_t clientTypeFlag)
{
    for (auto imUser : imUsers_) {
        if (imUser.second->isValidate()) {
            switch (clientTypeFlag) {
                case CLIENT_TYPE_FLAG_PC:
                    imUser.second->broadcastMsgWithOutMobile(message);
                    break;
                case CLIENT_TYPE_FLAG_MOBILE:
                    imUser.second->broadcastMsgToMobile(message);
                    break;
                case CLIENT_TYPE_FLAG_BOTH:
                    imUser.second->broadcastMsg(message);
                    break;
                default:
                    break;
            }
        }
    }
}

