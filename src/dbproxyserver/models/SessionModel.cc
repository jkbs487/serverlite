#include "SessionModel.h"

#include "MessageModel.h"
//#include "GroupMessageModel.h"

#include "slite/Logger.h"

using namespace slite;

SessionModel::SessionModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

SessionModel::~SessionModel()
{
}

void SessionModel::getRecentSession(uint32_t userId, uint32_t lastTime, list<IM::BaseDefine::ContactSessionInfo>& contacts)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "select * from IMRecentSession where userId = " + 
            std::to_string(userId) + " and status = 0 and updated >" + 
            std::to_string(lastTime) + " order by updated desc limit 100";
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            while (resultSet->next()) {
                IM::BaseDefine::ContactSessionInfo cRelate;
                uint32_t peerId = resultSet->getInt("peerId");
                cRelate.set_session_id(peerId);
                cRelate.set_session_status(::IM::BaseDefine::SessionStatusType(resultSet->getInt("status")));
                
                IM::BaseDefine::SessionType sessionType = IM::BaseDefine::SessionType(resultSet->getInt("type"));
                if (IM::BaseDefine::SessionType_IsValid(sessionType)) {
                    cRelate.set_session_type(IM::BaseDefine::SessionType(sessionType));
                    cRelate.set_updated_time(resultSet->getInt("updated"));
                    contacts.push_back(cRelate);
                } else {
                    LOG_WARN << "invalid sessionType. userId=" << userId << ", peerId=" << peerId 
                        << ", sessionType=" << sessionType;
                }
            }
            delete resultSet;
        } else {
            LOG_ERROR << "no result set for sql: " << strSql;
        }
        dbPool_->relDBConn(dbConn);
        if (!contacts.empty()) {
            fillSessionMsg(userId, contacts);
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
}

void SessionModel::fillSessionMsg(uint32_t userId, list<IM::BaseDefine::ContactSessionInfo>& contacts)
{
    for (auto it = contacts.begin(); it != contacts.end();) {
        uint32_t msgId = 0;
        string msgData;
        IM::BaseDefine::MsgType msgType;
        uint32_t fromId = 0;
        if (it->session_type() == IM::BaseDefine::SESSION_TYPE_SINGLE) {
            fromId = it->session_id();
            MessageModel messageModel(dbPool_, cachePool_);
            messageModel.getLastMsg(it->session_id(), userId, msgId, msgData, msgType);
        } else {
            //GroupMessageModel groupMessageModel(dbPool_, cachePool_);
            //groupMessageModel.getLastMsg(it->session_id(), msgId, msgData, msgType, fromId);
        }
        if (!IM::BaseDefine::MsgType_IsValid(msgType)) {
            it = contacts.erase(it);
        }
        else {
            it->set_latest_msg_from_user_id(fromId);
            it->set_latest_msg_id(msgId);
            it->set_latest_msg_data(msgData);
            it->set_latest_msg_type(msgType);
            ++it;
        }
    }
}

uint32_t SessionModel::getSessionId(uint32_t userId, uint32_t peerId, uint32_t type, bool isAll)
{
    DBConn* dbConn = dbPool_->getDBConn();
    uint32_t sessionId = 0;
    if(dbConn) {
        string strSql;
        if (isAll) {
            strSql= "SELECT id FROM IMRecentSession WHERE userId = " + 
                std::to_string(userId) + " and peerId=" + std::to_string(peerId) + 
                " and type=" + std::to_string(type);
        } else {
            strSql= "SELECT id FROM IMRecentSession WHERE userId = " + 
                std::to_string(userId) + " and peerId=" + std::to_string(peerId) + 
                " and type=" + std::to_string(type) + " and status=0";
        }
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            while (resultSet->next()) {
                sessionId = resultSet->getInt("id");
            }
            delete resultSet;
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    return sessionId;
}

bool SessionModel::updateSession(uint32_t sessionId, uint32_t updateTime)
{
    bool ret = false;
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "UPDATE IMRecentSession set `updated` = " + 
            std::to_string(updateTime) + " WHERE id=" + std::to_string(sessionId);
        ret = dbConn->executeUpdate(strSql);
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    return ret;
}

uint32_t SessionModel::addSession(uint32_t userId, uint32_t peerId, uint32_t type)
{
    uint32_t sessionId = 0;
    
    sessionId = getSessionId(userId, peerId, type, true);
    uint32_t timeNow = static_cast<uint32_t>(time(NULL));
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        if(0 != sessionId) {
            string strSql = "update IMRecentSession set status=0, updated=" + 
                std::to_string(timeNow) + " where id=" + std::to_string(sessionId);
            bool ret = dbConn->executeUpdate(strSql);
            if(!ret) {
                sessionId = 0;
            }
            LOG_WARN << "has relation ship set status";
        }
        else {
            string strSql = "insert into IMRecentSession (`userId`,`peerId`,`type`,`status`,`created`,`updated`) values(?,?,?,?,?,?)";
            // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
            PrepareStatement* stmt = new PrepareStatement();
            if (stmt->init(dbConn->getMysql(), strSql)) {
                uint32_t status = 0;
                uint32_t index = 0;
                stmt->setParam(index++, userId);
                stmt->setParam(index++, peerId);
                stmt->setParam(index++, type);
                stmt->setParam(index++, status);
                stmt->setParam(index++, timeNow);
                stmt->setParam(index++, timeNow);
                bool ret = stmt->executeUpdate();
                if (ret) {
                    sessionId = dbConn->getInsertId();
                } else {
                    LOG_ERROR << "insert message failed. " << strSql;
                }
            }
            delete stmt;
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    return sessionId;
}