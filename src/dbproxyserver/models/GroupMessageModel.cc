#include "GroupMessageModel.h"
#include "GroupModel.h"
#include "slite/Logger.h"

#define     GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX    "_im_group_msg"
#define     GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX     "_im_user_group"
#define     GROUP_COUNTER_SUBKEY_COUNTER_FIELD          "count"

using namespace slite;

GroupMessageModel::GroupMessageModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

GroupMessageModel::~GroupMessageModel()
{
}

/**
 *  获取一个群的最后一条消息
 *
 *  @param groupId   群Id
 *  @param msgId     最后一条消息的msgId,引用
 *  @param msgData 最后一条消息的内容,引用
 *  @param msgType   最后一条消息的类型,引用
 */
void GroupMessageModel::getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo>& unreadCounts)
{
    list<uint32_t> groupIds;
    GroupModel groupModel(dbPool_, cachePool_);
    groupModel.getUserGroupIds(userId, groupIds, 0);
    uint32_t count = 0;
    
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        for(auto groupId : groupIds) {
            string groupKey = std::to_string(groupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string groupCnt = cacheConn->hget(groupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            if (groupCnt.empty()) {
//                log("hget %s : count failed !", strGroupKey.c_str());
                continue;
            }
            uint32_t nGroupCnt = (uint32_t)(atoi(groupCnt.c_str()));
            
            string userKey = std::to_string(userId) + "_" + std::to_string(groupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string userCnt = cacheConn->hget(userKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            
            uint32_t nUserCnt = ( userCnt.empty() ? 0 : ((uint32_t)atoi(userCnt.c_str())) );
            if (nGroupCnt >= nUserCnt) {
                count = nGroupCnt - nUserCnt;
            }
            if (count > 0) {
                IM::BaseDefine::UnreadInfo unreadInfo;
                unreadInfo.set_session_id(groupId);
                unreadInfo.set_session_type(IM::BaseDefine::SESSION_TYPE_GROUP);
                unreadInfo.set_unread_cnt(count);
                totalCnt += count;
                string msgData;
                uint32_t msgId;
                IM::BaseDefine::MsgType msgType;
                uint32_t fromId;
                getLastMsg(groupId, msgId, msgData, msgType, fromId);
                if (IM::BaseDefine::MsgType_IsValid(msgType)) {
                    unreadInfo.set_latest_msg_id(msgId);
                    unreadInfo.set_latest_msg_data(msgData);
                    unreadInfo.set_latest_msg_type(msgType);
                    unreadInfo.set_latest_msg_from_user_id(fromId);
                    unreadCounts.push_back(unreadInfo);
                } else {
                    LOG_ERROR << "invalid msgType. userId=" << userId 
                        << ", groupId=" << groupId << ", msgType=" 
                        << msgType << ", msgId=%u" << msgId;
                }
            }
        }
        cachePool_->relCacheConn(cacheConn);
    } else {
        LOG_ERROR << "no cache connection for unread";
    }
}

/**
 *  获取一个群的最后一条消息
 *
 *  @param nGroupId   群Id
 *  @param nMsgId     最后一条消息的msgId,引用
 *  @param strMsgData 最后一条消息的内容,引用
 *  @param nMsgType   最后一条消息的类型,引用
 */
void GroupMessageModel::getLastMsg(uint32_t groupId, uint32_t &msgId, string &msgData, IM::BaseDefine::MsgType &msgType, uint32_t& fromId)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT msgId, type, userId, content FROM IMGroupMessage_" + std::to_string(groupId % 8) + 
            " WHERE groupId = " + std::to_string(groupId) + " AND status = 0 ORDER BY created DESC, id DESC LIMIT 1";
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            while(resultSet->next()) {
                msgId = resultSet->getInt("msgId");
                msgType = IM::BaseDefine::MsgType(resultSet->getInt("type"));
                fromId = resultSet->getInt("userId");
                if (msgType == IM::BaseDefine::MSG_TYPE_GROUP_AUDIO) {
                    // "[语音]"加密后的字符串
                    //msgData = audioEnc;
                } else {
                    msgData = resultSet->getString("content");
                }
            }
            delete resultSet;
        } else {
            LOG_ERROR << "no result set for sql: " << strSql.c_str();
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
}

/**
 *  获取群组消息列表
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *  @param nMsgId   开始的msgId(最新的msgId)
 *  @param nMsgCnt  获取的长度
 *  @param lsMsg    消息列表
 */
void GroupMessageModel::getMessage(uint32_t userId, uint32_t groupId, uint32_t msgId, uint32_t msgCnt, list<IM::BaseDefine::MsgInfo>& msgs)
{
    GroupModel groupModel(dbPool_, cachePool_);
    //根据 count 和 lastId 获取信息
    string tableName = "IMGroupMessage_" + std::to_string(groupId % 8);
    
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        uint32_t updated = groupModel.getUserJoinTime(groupId, userId);
        //如果nMsgId 为0 表示客户端想拉取最新的nMsgCnt条消息
        string strSql;
        if (msgId == 0) {
            strSql = "SELECT * FROM " + tableName + " WHERE groupId = " + std::to_string(groupId) + 
            " AND status = 0 AND created >= "+ std::to_string(updated) + 
            " ORDER BY created desc, id desc LIMIT " + std::to_string(msgCnt);
        } else {
            strSql = "SELECT * FROM " + tableName + " WHERE groupId = " + std::to_string(groupId) + 
            " AND msgId <= " + std::to_string(msgId) + " AND status = 0 AND created >= " + 
            std::to_string(updated) + " ORDER BY created desc, id desc LIMIT " + std::to_string(msgCnt);
        }
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            map<uint32_t, IM::BaseDefine::MsgInfo> mapAudioMsg;
            while (resultSet->next()) {
                IM::BaseDefine::MsgInfo msg;
                msg.set_msg_id(resultSet->getInt("msgId"));
                msg.set_from_session_id(resultSet->getInt("userId"));
                msg.set_create_time(resultSet->getInt("created"));
                IM::BaseDefine::MsgType msgType = IM::BaseDefine::MsgType(resultSet->getInt("type"));
                if (IM::BaseDefine::MsgType_IsValid(msgType)) {
                    msg.set_msg_type(msgType);
                    msg.set_msg_data(resultSet->getString("content"));
                    msgs.push_back(msg);
                } else {
                    LOG_ERROR << "invalid msgType. userId=" << userId << ", groupId=" 
                        << groupId << ", msgType=" << msgType;
                }
            }
            delete resultSet;
        } else {
            LOG_ERROR << "no result set for sql: " << strSql;
        }
        dbPool_->relDBConn(dbConn);
        if (!msgs.empty()) {
            //CAudioModel::getInstance()->readAudios(lsMsg);
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
}