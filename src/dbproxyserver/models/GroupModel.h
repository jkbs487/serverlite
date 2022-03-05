#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class GroupModel
{
public:
    GroupModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~GroupModel();

    void getUserGroup(uint32_t userId, list<IM::BaseDefine::GroupVersionInfo>& groups, uint32_t groupType);
    void getUserGroupIds(uint32_t nUserId, list<uint32_t>& lsGroupId, uint32_t nLimited = 100);
    void getGroupUser(uint32_t groupId, list<uint32_t>& userIds);

private:
    void getGroupVersion(list<uint32_t>&lsGroupId, list<IM::BaseDefine::GroupVersionInfo>& lsGroup, uint32_t nGroupType);

    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
