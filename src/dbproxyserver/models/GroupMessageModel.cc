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

uint32_t GroupMessageModel::getMsgId(uint32_t groupId)
{
    uint32_t msgId = 0;
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if(cacheConn) {
        string strKey = "group_msg_id_" + std::to_string(groupId);
        msgId =  static_cast<uint32_t>(cacheConn->incrBy(strKey, 1));
        cachePool_->relCacheConn(cacheConn);
    } else {
        LOG_ERROR << "no cache connection for unread";
    }
    return msgId;
}

bool GroupMessageModel::sendMessage(uint32_t fromId, uint32_t groupId, IM::BaseDefine::MsgType msgType,
                                     uint32_t createTime, uint32_t msgId, const string& msgContent)
{
    bool ret = false;
    GroupModel groupModel(dbPool_, cachePool_);
    if (groupModel.isInGroup(fromId, groupId)) {
        DBConn* dbConn = dbPool_->getDBConn();
        if (dbConn) {
            string tableName = "IMGroupMessage_" + std::to_string(groupId % 8);
            string strSql = "insert into " + tableName + " (`groupId`, `userId`, `msgId`, `content`, `type`, `status`, `updated`, `created`) "\
            "values(?, ?, ?, ?, ?, ?, ?, ?)";
            
            // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
            PrepareStatement* stmt = new PrepareStatement();
            if (stmt->init(dbConn->getMysql(), strSql)) {
                uint32_t nStatus = 0;
                uint32_t nType = msgType;
                uint32_t index = 0;
                stmt->setParam(index++, groupId);
                stmt->setParam(index++, fromId);
                stmt->setParam(index++, msgId);
                stmt->setParam(index++, msgContent);
                stmt->setParam(index++, nType);
                stmt->setParam(index++, nStatus);
                stmt->setParam(index++, createTime);
                stmt->setParam(index++, createTime);
                
                ret = stmt->executeUpdate();
                if (ret) {
                    groupModel.updateGroupChat(groupId);
                    incMessageCount(fromId, groupId);
                    clearMessageCount(fromId, groupId);
                } else {
                    LOG_ERROR << "insert message failed: " << strSql;
                }
            }
            delete stmt;
            dbPool_->relDBConn(dbConn);
        } else {
            LOG_ERROR << "no db connection for teamtalk_master";
        }
    } else {
        LOG_ERROR << "not in the group.fromId=" << fromId << ", groupId=" << groupId;
    }
    return ret;
}
/*
bool GroupMessageModel::sendAudioMessage(uint32_t fromId, uint32_t groupId, IM::BaseDefine::MsgType msgType, 
                    uint32_t createTime, uint32_t msgId,const char* msgContent, uint32_t msgLen)
{
	if (msgLen <= 4) {
		return false;
	}

    GroupModel groupModel(dbPool_, cachePool_);
    if (!groupModel.isInGroup(fromId, groupId)) {
        LOG_ERROR << "not in the group. fromId=" << fromId << ", groupId=" << groupId);
        return false;
    }
    
	AudioModel audioModel(dbPool_, cachePool_);
	int audioId = audioModel.saveAudioInfo(fromId, groupId, createTime, msgContent, msgLen);

	bool ret = true;
	if (audioId != -1) {
		string strMsg = std::to_string(audioId);
        ret = sendMessage(fromId, groupId, msgType, createTime, msgId, strMsg);
	} else {
		ret = false;
	}

	return ret;
}
*/
/**
 *  增加群消息计数
 *
 *  @param userId  用户Id
 *  @param groupId 群组Id
 *
 *  @return 成功返回true，失败返回false
 */
bool GroupMessageModel::incMessageCount(uint32_t userId, uint32_t groupId)
{
    bool ret = false;
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        string groupKey = std::to_string(groupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
        cacheConn->hincrBy(groupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD, 1);
        map<string, string> groupCount;
        ret = cacheConn->hgetAll(groupKey, groupCount);
        if (ret) {
            string userKey = std::to_string(userId) + "_" + std::to_string(groupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strReply = cacheConn->hmset(userKey, groupCount);
            if (!strReply.empty()) {
                ret = true;
            } else {
                LOG_ERROR << "hmset " << userKey << " failed!";
            }
        } else {
            LOG_ERROR << "hgetAll " << groupKey << " failed!";
        }
        cachePool_->relCacheConn(cacheConn);
    } else {
        LOG_ERROR << "no cache connection for unread";
    }
    return ret;
}

bool GroupMessageModel::clearMessageCount(uint32_t userId, uint32_t groupId)
{
    bool ret = false;
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        string groupKey = std::to_string(groupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
        map<string, string> groupCount;
        ret = cacheConn->hgetAll(groupKey, groupCount);
        if (ret) {
            string userKey = std::to_string(userId) + "_" + std::to_string(groupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strReply = cacheConn->hmset(userKey, groupCount);
            if(strReply.empty()) {
                LOG_ERROR << "hmset " << userKey << " failed!";
            } else {
                ret = true;
            }
        } else{
            LOG_ERROR << "hgetAll " << groupKey << " failed!";
        }
    }
    else {
        LOG_ERROR << "no cache connection for unread";
    }
    cachePool_->relCacheConn(cacheConn);
    return ret;
}

void GroupMessageModel::getUnReadCntAll(uint32_t userId, uint32_t &totalCnt)
{
    list<uint32_t> groupIds;
    GroupModel groupModel(dbPool_, cachePool_);
    groupModel.getUserGroupIds(userId, groupIds, 0);
    uint32_t count = 0;
    
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        for (const auto& groupId : groupIds) {
            string groupKey = std::to_string(groupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strGroupCnt = cacheConn->hget(groupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            if (strGroupCnt.empty()) {
//                log("hget %s : count failed !", strGroupKey.c_str());
                continue;
            }
            uint32_t groupCnt = static_cast<uint32_t>(std::stoi(strGroupCnt));
            string userKey = std::to_string(userId) + "_" + std::to_string(groupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
            string strUserCnt = cacheConn->hget(userKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
            
            uint32_t userCnt = (strUserCnt.empty() ? 0 : ((uint32_t)atoi(strUserCnt.c_str())) );
            if (groupCnt >= userCnt) {
                count = groupCnt - userCnt;
            }
            if (count > 0) {
                totalCnt += count;
            }
        }
        cachePool_->relCacheConn(cacheConn);
    } else {
        LOG_ERROR << "no cache connection for unread";
    }
}