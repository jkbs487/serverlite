/*
 * ImUser.h
 *
 *  Created on: 2014年4月16日
 *      Author: ziteng
 */

#ifndef IMUSER_H_
#define IMUSER_H_

#include "TCPConnection.h"
#include "public_define.h"

#define MAX_ONLINE_FRIEND_CNT		100	//通知好友状态通知的最多个数

#include "../protobuf/codec.h"

#include <map>
#include <set>
#include <list>
#include <string>

using namespace slite;

class ImUser
{
public:
    ImUser(std::string userName, ProtobufCodec codec);
    ~ImUser();
    
    void setUserId(uint32_t userId) { userId_ = userId; }
    uint32_t getUserId() { return userId_; }
    std::string getLoginName() { return loginName_; }
    void setNickName(std::string nickName) { nickName_ = nickName; }
    std::string getNickName() { return nickName_; }
    bool isValidate() { return validate_; }
    void setValidated() { validate_ = true; }
    uint32_t getPCLoginStatus() { return pcLoginStatus_; }
    void setPCLoginStatus(uint32_t pcLoginStatus) { pcLoginStatus_ = pcLoginStatus; }
    
    user_conn_t getUserConn();
    
    bool isMsgConnEmpty() { return conns_.empty(); }
    void addMsgConn(std::string name, TCPConnectionPtr msgConn) { conns_[name] = msgConn; }
    void delMsgConn(std::string name) { conns_.erase(name); }
    TCPConnectionPtr getMsgConn(std::string name);
    void validateMsgConn(std::string name, TCPConnectionPtr pMsgConn);
    
    void addUnValidateMsgConn(const TCPConnectionPtr& msgConn) { unvalidateConnSet_.insert(msgConn); }
    void delUnValidateMsgConn(const TCPConnectionPtr& msgConn) { unvalidateConnSet_.erase(msgConn); }
    TCPConnectionPtr getUnValidateMsgConn(std::string name);
    
    std::map<std::string, TCPConnectionPtr>& getMsgConnMap() { return conns_; }

    void broadcastMsg(MessagePtr message, TCPConnectionPtr fromConn = NULL);
    void broadcastMsgWithOutMobile(MessagePtr message, TCPConnectionPtr fromConn = NULL);
    void broadcastMsgToMobile(MessagePtr message, TCPConnectionPtr fromConn = NULL);
    void broadcastClientMsgData(MessagePtr message, uint32_t msg_id, TCPConnectionPtr fromConn = NULL, uint32_t from_id = 0);
    void broadcastData(void* buff, uint32_t len, TCPConnectionPtr fromConn = NULL);
        
    void handleKickUser(TCPConnectionPtr msgConn, uint32_t reason);
    
    bool kickOutSameClientType(uint32_t clientType, uint32_t reason, TCPConnectionPtr fromConn = NULL);
    
    uint32_t getClientTypeFlag();
private:
    uint32_t userId_;
    std::string	loginName_;            /* 登录名 */
    std::string nickName_;            /* 花名 */
    bool userUpdated_;
    uint32_t pcLoginStatus_;  // pc client login状态，1: on 0: off
    
    bool validate_;

    ProtobufCodec codec_;
    std::map<std::string, TCPConnectionPtr>	conns_;
    std::set<TCPConnectionPtr> unvalidateConnSet_;
};

typedef std::map<uint32_t /* user_id */, ImUser*> ImUserMap_t;
typedef std::map<std::string /* 登录名 */, ImUser*> ImUserMapByName_t;

class ImUserManager
{
public:
    ImUserManager() {}
    ~ImUserManager();
    
    static ImUserManager* getInstance();
    ImUser* getImUserById(uint32_t userId);
    ImUser* getImUserByLoginName(std::string loginName);
    
    TCPConnectionPtr getMsgConnByHandle(uint32_t userId, std::string name);
    bool addImUserByLoginName(std::string loginName, ImUser* pUser);
    void removeImUserByLoginName(std::string loginName);
    
    bool addImUserById(uint32_t userId, ImUser* pUser);
    void removeImUserById(uint32_t userId);
    
    void removeImUser(ImUser* pUser);
    
    void removeAll();
    void getOnlineUserInfo(std::list<user_stat_t>* online_user_info);
    void getUserConnCnt(std::list<user_conn_t>* user_conn_list, uint32_t& total_conn_cnt);
    
    void broadcastMsg(MessagePtr message, uint32_t clientTypeFlag);
private:
    ImUserMap_t imUsers_;
    ImUserMapByName_t imUsersByName_;
};

void get_online_user_info(list<user_stat_t>* onlineUserInfo);


#endif /* IMUSER_H_ */
