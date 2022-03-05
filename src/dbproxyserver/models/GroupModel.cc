#include "GroupModel.h"
#include "slite/Logger.h"

using namespace slite;

GroupModel::GroupModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

GroupModel::~GroupModel()
{
}

void GroupModel::getGroupUser(uint32_t groupId, list<uint32_t>& userIds)
{
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn)
    {
        string key = "group_member_" + std::to_string(groupId);
        map<string, string> mapAllUser;
        bool ret = cacheConn->hgetAll(key, mapAllUser);
        cachePool_->relCacheConn(cacheConn);
        if(ret) {
            for (const auto& mapUser : mapAllUser) {
                uint32_t userId = std::stoi(mapUser.first);
                userIds.push_back(userId);
            }
        } else {
            LOG_ERROR << "hgetall " << key << " failed!";
        }
    } else {
        LOG_ERROR << "no cache connection for group_member";
    }
}

void GroupModel::getUserGroup(uint32_t userId, list<IM::BaseDefine::GroupVersionInfo>& groups, uint32_t groupType)
{
    list<uint32_t> groupIds;
    getUserGroupIds(userId, groupIds, 0);
    if (groupIds.size() != 0)
    {
        getGroupVersion(groupIds, groups, groupType);
    }
}

void GroupModel::getUserGroupIds(uint32_t userId, list<uint32_t>& groupIds, uint32_t limited)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if(dbConn) {
        string strSql;
        if (limited != 0) {
            strSql = "SELECT groupId FROM IMGroupMember WHERE userId = " + 
                std::to_string(userId) + " AND status = 0 ORDER BY updated DESC, id DESC LIMIT " + std::to_string(limited);
        } else {
            strSql = "SELECT groupId FROM IMGroupMember WHERE userId = " + 
                std::to_string(userId) + " AND status = 0 ORDER BY updated DESC, id DESC";
        }
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            while (resultSet->next()) {
                uint32_t groupId = resultSet->getInt("groupId");
                groupIds.push_back(groupId);
            }
            delete resultSet;
        } else{
            LOG_ERROR << "no result set for sql: %s" << strSql;
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
}

void GroupModel::getGroupVersion(list<uint32_t>& groupIds, list<IM::BaseDefine::GroupVersionInfo>& groups, uint32_t groupType)
{
    if (groupIds.empty()) {
        LOG_ERROR << "group ids is empty";
        return;
    }
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string clause;
        bool first = true;
        for (const auto& groupId : groupIds) {
            if (first) {
                first = false;
                clause = std::to_string(groupId);
            } else {
                clause += ("," + std::to_string(groupId));
            }
        }
        
        string strSql = "SELECT id, version FROM IMGroup WHERE id IN (" +  clause  + ")";
        if(0 != groupType) {
            strSql += " AND type = " + std::to_string(groupType);
        }
        strSql += " ORDER BY updated DESC";
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            while(resultSet->next()) {
                IM::BaseDefine::GroupVersionInfo group;
                group.set_group_id(resultSet->getInt("id"));
                group.set_version(resultSet->getInt("version"));
                groups.push_back(group);
            }
            delete resultSet;
        } else {
            LOG_ERROR << "no result set for sql: %s" << strSql;
        }
        dbPool_->relDBConn(dbConn);
    }
    else {
        LOG_ERROR << "no db connection for teamtalk";
    }
}