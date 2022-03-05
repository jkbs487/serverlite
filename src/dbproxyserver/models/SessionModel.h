#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class SessionModel
{
public:
    SessionModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~SessionModel();

    void getRecentSession(uint32_t userId, uint32_t lastTime, list<IM::BaseDefine::ContactSessionInfo>& contacts);
    uint32_t getSessionId(uint32_t nUserId, uint32_t nPeerId, uint32_t nType, bool isAll);
    bool updateSession(uint32_t sessionId, uint32_t updateTime);
    uint32_t addSession(uint32_t nUserId, uint32_t nPeerId, uint32_t nType);


    void fillSessionMsg(uint32_t userId, list<IM::BaseDefine::ContactSessionInfo>& contacts);
private:
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
