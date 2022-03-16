#include "DBPool.h"
#include "CachePool.h"
#include "pbs/IM.BaseDefine.pb.h"

class MessageModel
{
public:
    MessageModel(DBPoolPtr dbPool, CachePoolPtr cachePool);
    ~MessageModel();

    void getMessage(uint32_t userId, uint32_t peerId, uint32_t msgId, uint32_t msgCnt, list<IM::BaseDefine::MsgInfo>& msgs);
    void getLastMsg(uint32_t fromId, uint32_t toId, uint32_t& msgId, string& msgData, IM::BaseDefine::MsgType & msgType, uint32_t status = 0);
    uint32_t getMsgId(uint32_t relateId);
    bool resetMsgId(uint32_t relateId);
    void getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo>& unreadCounts);
    void getUnReadCntAll(uint32_t userId, uint32_t &totalCnt);
    bool sendMessage(uint32_t relateId, uint32_t fromId, uint32_t toId, IM::BaseDefine::MsgType msgType, uint32_t createTime,
                     uint32_t msgId, string& msgContent);
private:
    void incMsgCount(uint32_t fromId, uint32_t toId);

    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
