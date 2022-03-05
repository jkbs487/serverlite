#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class MessageModel
{
public:
    MessageModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~MessageModel();

    void getLastMsg(uint32_t fromId, uint32_t toId, uint32_t& msgId, string& msgData, IM::BaseDefine::MsgType & msgType, uint32_t status = 0);
    bool resetMsgId(uint32_t relateId);
    void getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo>& unreadCounts);
private:
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
