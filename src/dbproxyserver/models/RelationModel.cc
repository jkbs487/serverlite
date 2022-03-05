#include "RelationModel.h"
#include "MessageModel.h"
#include "slite/Logger.h"

using namespace slite;

//extern string audioEnc;

RelationModel::RelationModel(DBPoolPtr dbPool, CachePoolPtr cachePool)
    : dbPool_(dbPool),
    cachePool_(cachePool)
{
}

RelationModel::~RelationModel()
{
}

uint32_t RelationModel::getRelationId(uint32_t userAId, uint32_t userBId, bool add)
{
    uint32_t relationId = 0;
    if (userAId == 0 || userBId == 0) {
        LOG_ERROR << "invalied user id:" << userAId << "->" << userBId;
        return relationId;
    }
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        uint32_t bigId = userAId > userBId ? userAId : userBId;
        uint32_t smallId = userAId > userBId ? userBId : userAId;
        string strSql = "SELECT id FROM IMRelationShip WHERE smallId=" + std::to_string(smallId) + 
            " AND bigId="+ std::to_string(bigId) + " and status = 0";
        
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if (resultSet) {
            while (resultSet->next()) {
                relationId = resultSet->getInt("id");
            }
            delete resultSet;
        } else {
            LOG_ERROR << "there is no result for sql: " << strSql;
        }
        dbPool_->relDBConn(dbConn);
        if (relationId == 0 && add) {
            relationId = addRelation(smallId, bigId);
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    return relationId;
}

uint32_t RelationModel::addRelation(uint32_t smallId, uint32_t bigId)
{
    uint32_t relationId = 0;
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        uint32_t timeNow = static_cast<uint32_t>(time(NULL));
        string strSql = "SELECT id FROM IMRelationShip WHERE smallId=" + 
            std::to_string(smallId) + " and bigId=" + std::to_string(bigId);
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet && resultSet->next()) {
            relationId = resultSet->getInt("id");
            strSql = "UPDATE IMRelationShip SET status=0, updated=" + 
                std::to_string(timeNow) + " WHERE id=" + std::to_string(relationId);
            bool ret = dbConn->executeUpdate(strSql);
            if(!ret) {
                relationId = 0;
            }
            LOG_INFO << "has relation ship set status";
            delete resultSet;
        } else {
            strSql = "INSERT INTO IMRelationShip (`smallId`,`bigId`,`status`,`created`,`updated`) values(?,?,?,?,?)";
            // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash
            PrepareStatement* stmt = new PrepareStatement();
            if (stmt->init(dbConn->getMysql(), strSql)) {
                uint32_t status = 0;
                uint32_t index = 0;
                stmt->setParam(index++, smallId);
                stmt->setParam(index++, bigId);
                stmt->setParam(index++, status);
                stmt->setParam(index++, timeNow);
                stmt->setParam(index++, timeNow);
                bool ret = stmt->executeUpdate();
                if (ret) {
                    relationId = dbConn->getInsertId();
                } else {
                    LOG_ERROR << "insert message failed. %s" << strSql;
                }
            }
            if (relationId != 0) {
                // 初始化msgId
                MessageModel messageModel(dbPool_, cachePool_);
                if (messageModel.resetMsgId(relationId)) {
                    LOG_ERROR << "reset msgId failed. smallId=" << smallId << ", bigId=" << bigId;
                }
            }
            delete stmt;
        }
        dbPool_->relDBConn(dbConn);
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    return relationId;
}