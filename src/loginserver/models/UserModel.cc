#include "Logger.h"
#include "UserModel.h"

using namespace slite;

UserModel::UserModel(DBPoolPtr dbPool)
    : dbPool_(dbPool)
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

void UserModel::getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo>& lsUsers)
{
    if (lsIds.empty()) {
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

    for (auto lsId: lsIds) {
        if (first) {
            first = false;
            clause += std::to_string(lsId);
        } else {
            clause += ("," + std::to_string(lsId));
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
            lsUsers.push_back(cUser);
        }
        delete resultSet;
    } else {
        LOG_ERROR << "no result set for sql: " << strSql;
    }
    dbPool_->relDBConn(dbConn);
}