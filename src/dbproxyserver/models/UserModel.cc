#include "slite/Logger.h"
#include "UserModel.h"

#define     GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX    "_im_group_msg"
#define     GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX     "_im_user_group"
#define     GROUP_COUNTER_SUBKEY_COUNTER_FIELD          "count"

using namespace slite;

UserModel::UserModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

UserModel::~UserModel()
{
}

void UserModel::getChangedId(uint32_t& nLastTime, list<uint32_t>& lsIds)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if (!dbConn) {
        LOG_ERROR << "no db connection for teamtalk";
        return;
    }
    
    std::string strSql;
    if (nLastTime == 0)
        strSql = "SELECT id, updated FROM IMUser WHERE status != 3";
    else
        strSql = "SELECT id, updated FROM IMUser WHERE updated >=" + std::to_string(nLastTime);

    ResultSet* resultSet = dbConn->executeQuery(strSql);
    if (resultSet) {
        while (resultSet->next()) {
            uint32_t nId = resultSet->getInt("id");
            uint32_t nUpdated = resultSet->getInt("updated");
            if (nLastTime < nUpdated) {
                nLastTime = nUpdated;
            }
            lsIds.push_back(nId);
        }
        delete resultSet;
    } else {
        LOG_ERROR << "no result set for sql: " << strSql;
    }
    dbPool_->relDBConn(dbConn);
}

void UserModel::getUsers(list<uint32_t> ids, list<IM::BaseDefine::UserInfo>& users)
{
    if (ids.empty()) {
        LOG_WARN << "list is empty";
        return;
    }

    DBConn* dbConn = dbPool_->getDBConn();
    if (!dbConn) {
        LOG_ERROR << "no db connection for teamtalk";
        return;
    }

    std::string clause;
    bool first = true;

    for (auto id: ids) {
        if (first) {
            first = false;
            clause += std::to_string(id);
        } else {
            clause += ("," + std::to_string(id));
        }
    }
    std::string strSql = "SELECT * FROM IMUser WHERE id IN (" + clause + ")";
    ResultSet* resultSet = dbConn->executeQuery(strSql);
    if (resultSet) {
        while (resultSet->next()) {
            IM::BaseDefine::UserInfo cUser;
            cUser.set_user_id(resultSet->getInt("id"));
            cUser.set_user_gender(resultSet->getInt("sex"));
            cUser.set_user_nick_name(resultSet->getString("nick"));
            cUser.set_user_domain(resultSet->getString("domain"));
            cUser.set_user_real_name(resultSet->getString("name"));
            cUser.set_user_tel(resultSet->getString("phone"));
            cUser.set_email(resultSet->getString("email"));
            cUser.set_avatar_url(resultSet->getString("avatar"));
            cUser.set_sign_info(resultSet->getString("sign_info"));
            cUser.set_department_id(resultSet->getInt("departId"));
            cUser.set_department_id(resultSet->getInt("departId"));
            cUser.set_status(resultSet->getInt("status"));
            users.push_back(cUser);
        }
        delete resultSet;
    } else {
        LOG_ERROR << "no result set for sql: " << strSql;
    }
    dbPool_->relDBConn(dbConn);
}

// 获取勿扰模式
bool UserModel::getPushShield(uint32_t userId, uint32_t* shieldStatus)
{
    bool ret = false;
    
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT push_shield_status FROM IMUser WHERE id = " + std::to_string(userId);
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            if (resultSet->next()) {
                *shieldStatus = resultSet->getInt("push_shield_status");
                ret = true;
            }
            delete resultSet;
        } else {
            LOG_ERROR << "getPushShield: no result set for sql:" << strSql;
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "getPushShield: no db connection for teamtalk_slave";
    }
    
    return ret;
}

void UserModel::clearUserCounter(uint32_t userId, uint32_t peerId, IM::BaseDefine::SessionType sessionType)
{
    if (IM::BaseDefine::SessionType_IsValid(sessionType))
    {
        CacheConn* cacheConn = cachePool_->getCacheConn();
        if (cacheConn) {
            // Clear P2P msg Counter
            if (sessionType == IM::BaseDefine::SESSION_TYPE_SINGLE) {
                long ret = cacheConn->hdel("unread_" + std::to_string(userId), std::to_string(peerId));
                if (!ret) {
                    LOG_ERROR << "hdel failed " << peerId << "->" << userId;
                }
            }
            // Clear Group msg Counter
            else if (sessionType == IM::BaseDefine::SESSION_TYPE_GROUP) {
                string groupKey = std::to_string(peerId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
                map<string, string> mapGroupCount;
                bool ret = cacheConn->hgetAll(groupKey, mapGroupCount);
                if(ret)
                {
                    string userKey = std::to_string(userId) + "_" + std::to_string(peerId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
                    string strReply = cacheConn->hmset(userKey, mapGroupCount);
                    if (strReply.empty()) {
                        LOG_ERROR << "hmset " << userKey << " failed!";
                    }
                } else {
                    LOG_ERROR << "hgetall " << groupKey << " failed!";
                } 
            }
            cachePool_->relCacheConn(cacheConn);
        } else {
            LOG_ERROR << "no cache connection for unread";
        }
    }
    else {
        LOG_ERROR << "invalid sessionType. userId=%u, fromId=%u, sessionType=%u";
    }
}