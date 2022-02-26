#include "DepartmentModel.h"
#include "slite/Logger.h"

using namespace std;
using namespace slite;

DepartmentModel::DepartmentModel(DBPoolPtr dbPool)
    :dbPool_(dbPool)
{
}

DepartmentModel::~DepartmentModel()
{
}

void DepartmentModel::getChgedDeptId(uint32_t& nLastTime, list<uint32_t>& lsChangedIds)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT id, updated FROM IMDepart WHERE updated > " + std::to_string(nLastTime);
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            while (resultSet->next()) {
                uint32_t id = resultSet->getInt("id");
                uint32_t nUpdated = resultSet->getInt("updated");
                if(nLastTime < nUpdated) {
                    nLastTime = nUpdated;
                }
                lsChangedIds.push_back(id);
            }
            delete resultSet;
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    dbPool_->relDBConn(dbConn);
}

void DepartmentModel::getDepts(list<uint32_t>& lsDeptIds, list<IM::BaseDefine::DepartInfo>& lsDepts)
{
    if(lsDeptIds.empty()) {
        LOG_WARN << "list is empty";
        return;
    }
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strClause;
        bool bFirst = true;
        for (auto it=lsDeptIds.begin(); it!=lsDeptIds.end(); ++it) {
            if (bFirst) {
                bFirst = false;
                strClause += std::to_string(*it);
            } else {
                strClause += ("," + std::to_string(*it));
            }
        }
        string strSql = "SELECT * FROM IMDepart WHERE id IN ( " + strClause + " )";
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            while (resultSet->next()) {
                IM::BaseDefine::DepartInfo cDept;
                uint32_t nId = resultSet->getInt("id");
                uint32_t nParentId = resultSet->getInt("parentId");
                string strDeptName = resultSet->getString("departName");
                uint32_t nStatus = resultSet->getInt("status");
                uint32_t nPriority = resultSet->getInt("priority");
                if(IM::BaseDefine::DepartmentStatusType_IsValid(nStatus)) {
                    cDept.set_dept_id(nId);
                    cDept.set_parent_dept_id(nParentId);
                    cDept.set_dept_name(strDeptName);
                    cDept.set_dept_status(IM::BaseDefine::DepartmentStatusType(nStatus));
                    cDept.set_priority(nPriority);
                    lsDepts.push_back(cDept);
                }
            }
            delete resultSet;
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    dbPool_->relDBConn(dbConn);
}

void DepartmentModel::getDept(uint32_t nDeptId, IM::BaseDefine::DepartInfo& cDept)
{
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT * FROM IMDepart WHERE id = " + std::to_string(nDeptId);
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            while (resultSet->next()) {
                uint32_t nId = resultSet->getInt("id");
                uint32_t nParentId = resultSet->getInt("parentId");
                string strDeptName = resultSet->getString("departName");
                uint32_t nStatus = resultSet->getInt("status");
                uint32_t nPriority = resultSet->getInt("priority");
                if(IM::BaseDefine::DepartmentStatusType_IsValid(nStatus)) {
                    cDept.set_dept_id(nId);
                    cDept.set_parent_dept_id(nParentId);
                    cDept.set_dept_name(strDeptName);
                    cDept.set_dept_status(IM::BaseDefine::DepartmentStatusType(nStatus));
                    cDept.set_priority(nPriority);
                }
            }
            delete  resultSet;
        }
    } else {
        LOG_ERROR << "no db connection for teamtalk";
    }
    dbPool_->relDBConn(dbConn);
}
