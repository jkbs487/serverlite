#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class GroupMessageModel
{
public:
    GroupMessageModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~GroupMessageModel();

    void getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo>& unreadCounts);
    void getLastMsg(uint32_t groupId, uint32_t &msgId, string &msgData, IM::BaseDefine::MsgType &msgType, uint32_t& fromId);
    void getMessage(uint32_t userId, uint32_t groupId, uint32_t msgId, uint32_t msgCnt, list<IM::BaseDefine::MsgInfo>& msgs);

private:
    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
