#include "SyncCenter.h"

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sys/signal.h>
//#include "HttpClient.h"
#include "nlohmann/json.hpp"

#include "slite/Logger.h"
//#include "models/Common.h"
#include "models/UserModel.h"
#include "models/GroupModel.h"
#include "models/SessionModel.h"

using namespace slite;

/**
 *  构造函数
 */
SyncCenter::SyncCenter(CachePoolPtr cachePool, DBPoolPtr dbPool)
    ://groupChatThreadId_(0),
    lastUpdateGroup_(static_cast<uint32_t>(time(NULL))),
    syncGroupChatWaitting_(true),
    cachePool_(cachePool),
    dbPool_(dbPool)
{
}

/**
 *  析构函数
 */
SyncCenter::~SyncCenter()
{
}

void SyncCenter::getDept(uint32_t deptId, DBDeptInfo_t** dept)
{
    auto it = deptInfo_->find(deptId);
    if (it != deptInfo_->end()) {
        *dept = it->second;
    }
}

string SyncCenter::getDeptName(uint32_t nDeptId)
{
    std::shared_lock<std::shared_mutex> lock(deptMutex_); 
    string strDeptName;
    DBDeptInfo_t* pDept = nullptr;;
    getDept(nDeptId, &pDept);
    if (pDept != nullptr) {
        strDeptName =  pDept->strName;
    }
    return strDeptName;
}
/**
 *  开启内网数据同步以及群组聊天记录同步
 */
void SyncCenter::startSync()
{
    thread_ = std::thread(std::bind(&SyncCenter::doSyncGroupChat, this));
}

/**
 *  停止同步，为了"优雅"的同步，使用了条件变量
 */
void SyncCenter::stopSync()
{
    syncGroupChatWaitting_ = false;
    cond_.notify_all();
    while (syncGroupChatRuning_) {
        usleep(500);
    }
    thread_.join();
}

/*
 * 初始化函数，从cache里面加载上次同步的时间信息等
 */
void SyncCenter::init()
{
    // Load total update time
    // increase message count
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        string strTotalUpdate = cacheConn->get("total_user_updated");
        string strLastUpdateGroup = cacheConn->get("last_update_group");
  
        cachePool_->relCacheConn(cacheConn);
        if(!strTotalUpdate.empty()) {
            lastUpdate_ = std::stoi(strTotalUpdate);
        } else {
                updateTotalUpdate(static_cast<uint32_t>(time(NULL)));
        }
        if (!strLastUpdateGroup.empty()) {
            lastUpdateGroup_ = std::stoi(strLastUpdateGroup);
        } else {
            updateLastUpdateGroup(static_cast<uint32_t>(time(NULL)));
        }
    } else {
        LOG_ERROR << "no cache connection to get total_user_updated";
    }
}
/**
 *  更新上次同步内网信息时间
 *
 *  @param nUpdated 时间
 */

void SyncCenter::updateTotalUpdate(uint32_t updated)
{
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        //lock
        {
            lastUpdate_ = updated;
        }
        
        string strUpdated = std::to_string(updated);
        cacheConn->set("total_user_update", strUpdated);
        cachePool_->relCacheConn(cacheConn);
    }
    else
    {
        LOG_ERROR << "no cache connection to get total_user_updated";
    }
}

/**
 *  更新上次同步群组信息时间
 *
 *  @param nUpdated 时间
 */
void SyncCenter::updateLastUpdateGroup(uint32_t updated)
{
    CacheConn* cacheConn = cachePool_->getCacheConn();
    if (cacheConn) {
        //lock
        lastUpdate_ = updated;
        string strUpdated = std::to_string(updated);
        cacheConn->set("last_update_group", strUpdated);
        cachePool_->relCacheConn(cacheConn);
    }
    else
    {
        LOG_ERROR << "no cache connection to get total_user_updated";
    }
}

/**
 *  同步群组聊天信息
 *
 *  @param arg NULL
 *
 *  @return NULL
 */

void SyncCenter::doSyncGroupChat()
{
    syncGroupChatRuning_ = true;
    GroupModel groupModel(dbPool_, cachePool_);
    SessionModel sessionModel(dbPool_, cachePool_);
    map<uint32_t, uint32_t> mapChangedGroup;

    do {
        std::unique_lock<std::mutex> lock(groupChatMutex_);
        cond_.wait_for(lock, std::chrono::seconds(5), [this]{
            return !syncGroupChatWaitting_;
        });
        mapChangedGroup.clear();
        DBConn* dbConn = dbPool_->getDBConn();
        if(dbConn) {
            string strSql = "SELECT id, lastChated FROM IMGroup WHERE status=0 AND lastChated >= " + std::to_string(getLastUpdateGroup());
            ResultSet* result = dbConn->executeQuery(strSql);
            if(result) {
                while (result->next()) {
                    uint32_t groupId = result->getInt("id");
                    uint32_t lastChat = result->getInt("lastChated");
                    if (lastChat != 0) {   
                        mapChangedGroup[groupId] = lastChat;
                    }
                }
                delete result;
            }
            dbPool_->relDBConn(dbConn);
        } else {
            LOG_ERROR << "no db connection for teamtalk";
        }
        updateLastUpdateGroup(static_cast<uint32_t>(time(NULL)));
        for (const auto& item : mapChangedGroup) {
            uint32_t groupId = item.first;
            list<uint32_t> users;
            uint32_t update = item.second;
            groupModel.getGroupUser(groupId, users);
            for (const auto& user : users) {
                uint32_t userId = user;
                uint32_t sessionId = 0;
                sessionId = sessionModel.getSessionId(userId, groupId, IM::BaseDefine::SESSION_TYPE_GROUP, true);
                if (sessionId != 0) {
                    sessionModel.updateSession(sessionId, update);
                } else {
                    sessionModel.addSession(userId, groupId, IM::BaseDefine::SESSION_TYPE_GROUP);
                }
            }
        }
    } while (syncGroupChatWaitting_);

    syncGroupChatRuning_ = false;
}
