#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class RelationModel
{
public:
    RelationModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~RelationModel();

    uint32_t getRelationId(uint32_t userAId, uint32_t userBId, bool add);
    uint32_t addRelation(uint32_t nSmallId, uint32_t nBigId);

private:
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
