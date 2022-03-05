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