#pragma once

#include <set>
#include <string>

#define MAX_MSG_CNT_PER_SECOND			20	// user can not send more than 20 msg in one second

class ClientConnInfo {
public:
    void setUserId(int32_t userId) { userId_ = userId; }
    void setLastRecvTick(int64_t lastRecvTick) { lastRecvTick_ = lastRecvTick; }
    void setLastSendTick(int64_t lastSendTick) { lastSendTick_ = lastSendTick; }
    void setClientVersion(std::string version) { clientVersion_ = version; }
    void setLoginName(std::string loginName) { loginName_ = loginName; }
    void setClientType(uint32_t type) { clientType_ = type; }
    void setOnlineStatus(uint32_t onlineStatus) { onlineStatus_ = onlineStatus; }
    void incrMsgCntPerSec() { ++msgCntPerSec_; }
    void setKickOff() { kickOff_ = true; }
    void setOpen() { isOpen_ = true; } 
    void addMsgToSendList(uint32_t msgId, uint32_t fromId) 
    { sendList_.insert(std::pair<uint32_t, uint32_t>(msgId, fromId)); }
    void delMsgFromSendList(uint32_t msgId, uint32_t fromId) 
    { sendList_.erase(std::pair<uint32_t, uint32_t>(msgId, fromId)); }

    int32_t userId() { return userId_; }
    int64_t lastRecvTick() { return lastRecvTick_; }
    int64_t lastSendTick() { return lastSendTick_; }
    std::string clientVersion() { return clientVersion_; }
    std::string loginName() { return loginName_; }
    uint32_t clientType() { return clientType_; }
    uint32_t onlineStatus() { return onlineStatus_; }
    uint32_t msgCntPerSec() { return msgCntPerSec_; }
    bool isKickOff() { return kickOff_; }
    bool isOpen() { return isOpen_; }

private:
    int32_t userId_;
    int64_t lastRecvTick_;
    int64_t lastSendTick_;
    std::string clientVersion_;
    std::string loginName_;
    uint32_t clientType_;
    uint32_t onlineStatus_;
    uint32_t msgCntPerSec_;
    bool kickOff_;
    bool isOpen_;
    std::set<std::pair<uint32_t, uint32_t>> sendList_;
};