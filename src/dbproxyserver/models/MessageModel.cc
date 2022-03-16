#include "MessageModel.h"
#include "RelationModel.h"
#include "slite/Logger.h"

using namespace slite;

//extern string audioEnc;

MessageModel::MessageModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

MessageModel::~MessageModel()
{
}

void MessageModel::getLastMsg(uint32_t fromId, uint32_t toId, uint32_t& msgId, string& msgData, IM::BaseDefine::MsgType & msgType, uint32_t status)
{
    RelationModel relationModel(dbPool_, cachePool_);
    uint32_t relateId = relationModel.getRelationId(fromId, toId, false);
    
    if (relateId != 0) {
        DBConn* dbConn = dbPool_->getDBConn();
        if (dbConn) {
            string tableName = "IMMessage_" + std::to_string(relateId % 8);
            string strSql = "SELECT msgId, type, content FROM " + tableName + 
                " force index (idx_relateId_status_created) WHERE relateId = " + 
                std::to_string(relateId) + " AND status = 0 ORDER BY created DESC, id DESC LIMIT 1";
            ResultSet* resultSet = dbConn->executeQuery(strSql);
            if (resultSet) {
                while (resultSet->next()) {
                    msgId = resultSet->getInt("msgId");
                    msgType = IM::BaseDefine::MsgType(resultSet->getInt("type"));
                    if (msgType == IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO) {
                        // "[语音]"加密后的字符串 TODO
                        //msgData = audioEnc;
                    }
                    else {
                        msgData = resultSet->getString("content");
                    }
                }
                delete resultSet;
            } else {
                LOG_ERROR << "no result set: " << strSql;
            }
            dbPool_->relDBConn(dbConn);
        } else {
            LOG_ERROR << "no db connection_slave";
        }
    } else {
        LOG_ERROR << "no relation between " << fromId << " and " << toId;
    }
}

bool MessageModel::resetMsgId(uint32_t relateId)
{
    bool ret = false;
    //uint32_t msgId = 0;
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        string strKey = "msg_id_" + std::to_string(relateId);
        string strValue = "0";
        string strReply = cacheConn->set(strKey, strValue);
        if (strReply == strValue) {
            ret = true;
        }
        cachePool_->relCacheConn(cacheConn);
    }
    return ret;
}

uint32_t MessageModel::getMsgId(uint32_t relateId)
{
    uint32_t msgId = 0;
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        string strKey = "msg_id_" + std::to_string(relateId);
        msgId = static_cast<uint32_t>(cacheConn->incrBy(strKey, 1));
        cachePool_->relCacheConn(cacheConn);
    }
    return msgId;
}

void MessageModel::getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo>& unreadCounts)
{
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        map<string, string> unreads;
        string key = "unread_" + std::to_string(userId);
        bool ret = cacheConn->hgetAll(key, unreads);
        cachePool_->relCacheConn(cacheConn);
        if (ret) {
            IM::BaseDefine::UnreadInfo unreadInfo;
            for (const auto& unread : unreads) {
                unreadInfo.set_session_id(atoi(unread.first.c_str()));
                unreadInfo.set_unread_cnt(atoi(unread.second.c_str()));
                unreadInfo.set_session_type(IM::BaseDefine::SESSION_TYPE_SINGLE);
                uint32_t msgId = 0;
                string msgData;
                IM::BaseDefine::MsgType msgType;
                getLastMsg(unreadInfo.session_id(), userId, msgId, msgData, msgType);
                if (IM::BaseDefine::MsgType_IsValid(msgType)) {
                    unreadInfo.set_latest_msg_id(msgId);
                    unreadInfo.set_latest_msg_data(msgData);
                    unreadInfo.set_latest_msg_type(msgType);
                    unreadInfo.set_latest_msg_from_user_id(unreadInfo.session_id());
                    unreadCounts.push_back(unreadInfo);
                    totalCnt += unreadInfo.unread_cnt();
                } else {
                    LOG_ERROR << "invalid msgType. userId=" << userId 
                        << ", peerId=" << unreadInfo.session_id() << ", msgType=" << msgType;
                }
            }
        } else {
            LOG_ERROR << "hgetall " << key << " failed!";
        }
    } else {
        LOG_ERROR << "no cache connection for unread";
    }
}

void MessageModel::getMessage(uint32_t userId, uint32_t peerId, uint32_t msgId, uint32_t msgCnt, list<IM::BaseDefine::MsgInfo>& msgs)
{
    RelationModel relateModel(dbPool_, cachePool_);
    uint32_t relateId = relateModel.getRelationId(userId, peerId, false);
	if (relateId != 0) {
        DBConn* dbConn = dbPool_->getDBConn();
        if (dbConn) {
            string tableName = "IMMessage_" + std::to_string(relateId % 8);
            string strSql;
            if (msgId == 0) {
                strSql = "SELECT * FROM " + tableName + " force index (idx_relateId_status_created) WHERE relateId = " + 
                    std::to_string(relateId) + " AND status = 0 order by created desc, id desc limit " + std::to_string(msgCnt);
            } else {
                strSql = "SELECT * FROM " + tableName + " force index (idx_relateId_status_created) WHERE relateId = " + 
                    std::to_string(relateId) + " AND status = 0 AND msgId <=" + std::to_string(msgId)+ 
                    " order by created desc, id desc limit " + std::to_string(msgCnt);
            }
            ResultSet* resultSet = dbConn->executeQuery(strSql);
            if (resultSet) {
                while (resultSet->next()) {
                    IM::BaseDefine::MsgInfo msg;
                    msg.set_msg_id(resultSet->getInt("msgId"));
                    msg.set_from_session_id(resultSet->getInt("fromId"));
                    msg.set_create_time(resultSet->getInt("created"));
                    IM::BaseDefine::MsgType msgType = IM::BaseDefine::MsgType(resultSet->getInt("type"));
                    if (IM::BaseDefine::MsgType_IsValid(msgType)) {
                        msg.set_msg_type(msgType);
                        msg.set_msg_data(resultSet->getString("content"));
                        msgs.push_back(msg);
                    } else {
                        LOG_ERROR << "invalid msgType. userId=" << userId << ", peerId=" << peerId 
                            << ", msgId=" << msgId << ", msgCnt=" << msgId << ", msgType=" << msgType;
                    }
                }
                delete resultSet;
            } else {
                LOG_ERROR << "no result set: " << strSql;
            }
            dbPool_->relDBConn(dbConn);
            if (!msgs.empty()) {
                //CAudioModel::getInstance()->readAudios(lsMsg);
            }
        } else {
            LOG_ERROR << "no db connection for teamtalk";
        }
	} else {
        LOG_ERROR << "no relation between " << userId << " and " << peerId;
    }
}


/*
 * IMMessage 分表
 * AddFriendShip()
 * if fromId or toId is ShopEmployee
 * GetShopId
 * Insert into IMMessage_ShopId%8
 */
bool MessageModel::sendMessage(uint32_t relateId, uint32_t fromId, uint32_t toId, IM::BaseDefine::MsgType msgType, uint32_t createTime,
                     uint32_t msgId, string& msgContent)
{
    bool ret = false;
    if (fromId == 0 || toId == 0) {
        LOG_ERROR << "invalied userId." << fromId << "->" << toId;
        return ret;
    }
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string tableName = "IMMessage_" + std::to_string(relateId % 8);
        string strSql = "insert into " + tableName + " (`relateId`, `fromId`, `toId`, `msgId`, `content`, `status`, `type`, `created`, `updated`) values(?, ?, ?, ?, ?, ?, ?, ?, ?)";
        // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
        PrepareStatement* stmt = new PrepareStatement();
        if (stmt->init(dbConn->getMysql(), strSql)) {
            uint32_t status = 0;
            uint32_t type = msgType;
            uint32_t index = 0;
            stmt->setParam(index++, relateId);
            stmt->setParam(index++, fromId);
            stmt->setParam(index++, toId);
            stmt->setParam(index++, msgId);
            stmt->setParam(index++, msgContent);
            stmt->setParam(index++, status);
            stmt->setParam(index++, type);
            stmt->setParam(index++, createTime);
            stmt->setParam(index++, createTime);
            
            ret = stmt->executeUpdate();
            if (ret) {
                //uint32_t now = static_cast<uint32_t>(time(NULL));
                LOG_DEBUG << "sql: " << strSql << ", insert Id: " << stmt->getInsertId();
                incMsgCount(fromId, toId);
            } else {
                LOG_ERROR << "insert message failed: " << strSql;
            }
        }
        delete stmt;
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk_master";
    }

    return ret;
}

void MessageModel::incMsgCount(uint32_t fromId, uint32_t toId)
{
	CacheConn* cacheConn = cachePool_->getCacheConn();
	if (cacheConn) {
		cacheConn->hincrBy("unread_" + std::to_string(toId), std::to_string(fromId), 1);
		cachePool_->relCacheConn(cacheConn);
	} else {
		LOG_ERROR << "no cache connection to increase unread count: " << fromId << "->%d" << toId;
	}
}

void MessageModel::getUnReadCntAll(uint32_t userId, uint32_t &totalCnt)
{
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        map<string, string> mapUnread;
        string key = "unread_" + std::to_string(userId);
        bool ret = cacheConn->hgetAll(key, mapUnread);
        cachePool_->relCacheConn(cacheConn);
        
        if (ret) {
            for (auto it = mapUnread.begin(); it != mapUnread.end(); it++) {
                totalCnt += std::stoi(it->second);
            }
        } else {
            LOG_ERROR << "hgetall " << key << " failed!";
        }
    } else {
        LOG_ERROR << "no cache connection for unread";
    }   
}