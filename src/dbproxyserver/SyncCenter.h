#pragma once

#include <list>
#include <map>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

#include "base/public_define.h"
#include "pbs/IM.BaseDefine.pb.h"

#include "models/DBPool.h"
#include "models/CachePool.h"

class SyncCenter
{
public:
    SyncCenter(CachePoolPtr cachePool, DBPoolPtr dbPool);
    ~SyncCenter();
    
    uint32_t getLastUpdate() {           
        std::lock_guard<std::mutex> lock(lastUpdateMutex_);
        return lastUpdate_;
    }
      uint32_t getLastUpdateGroup() {
        std::lock_guard<std::mutex> lock(lastUpdateMutex_);
        return lastUpdateGroup_;
    }
    string getDeptName(uint32_t nDeptId);
    void startSync();
    void stopSync();
    void init();
    void updateTotalUpdate(uint32_t nUpdated);

private:
    SyncCenter(const SyncCenter& sync) = delete;
    SyncCenter& operator=(const SyncCenter& sync) = delete;
    void updateLastUpdateGroup(uint32_t nUpdated);
    void doSyncGroupChat();
    
private:
    void getDept(uint32_t nDeptId, DBDeptInfo_t** pDept);
    DBDeptMap_t* deptInfo_;

    uint32_t lastUpdateGroup_;
    uint32_t lastUpdate_;

    std::condition_variable cond_;
    std::mutex groupChatMutex_;
    bool syncGroupChatRuning_;
    bool syncGroupChatWaitting_;
    std::thread thread_;
    std::mutex lastUpdateMutex_;
    std::shared_mutex deptMutex_;
    CachePoolPtr cachePool_;
    DBPoolPtr dbPool_;
};