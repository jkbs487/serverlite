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