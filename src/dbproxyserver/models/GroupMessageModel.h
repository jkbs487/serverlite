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
    uint32_t getMsgId(uint32_t groupId);
    bool sendMessage(uint32_t fromId, uint32_t groupId, IM::BaseDefine::MsgType msgType, uint32_t createTime, uint32_t msgId, const string& msgContent);
    bool sendAudioMessage(uint32_t fromId, uint32_t groupId, IM::BaseDefine::MsgType msgType, uint32_t createTime, uint32_t msgId,const char* msgContent, uint32_t msgLen);
    bool clearMessageCount(uint32_t userId, uint32_t groupId);
    void getUnReadCntAll(uint32_t userId, uint32_t &totalCnt);
private:
    bool incMessageCount(uint32_t userId, uint32_t groupId);

    DBPoolPtr dbPool_;
    CachePoolPtr cachePool_;
};
