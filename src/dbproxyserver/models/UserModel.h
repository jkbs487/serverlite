#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class UserModel
{
public:
    UserModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~UserModel();

    void getChangedId(uint32_t& nLastTime, list<uint32_t>& lsIds);
    void getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo>& lsUsers);
    bool getPushShield(uint32_t userId, uint32_t* shieldStatus);
    void clearUserCounter(uint32_t userId, uint32_t peerId, IM::BaseDefine::SessionType sessionType);

private:
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
