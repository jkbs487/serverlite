#include <memory>

#include "pbs/IM.File.pb.h"
#include "pbs/IM.Login.pb.h"
#include "pbs/IM.Server.pb.h"
#include "pbs/IM.Buddy.pb.h"
#include "pbs/IM.Other.pb.h"
#include "pbs/IM.Group.pb.h"
#include "pbs/IM.Message.pb.h"
#include "pbs/IM.SwitchService.pb.h"

typedef std::shared_ptr<IM::Login::IMLoginReq> LoginReqPtr;
typedef std::shared_ptr<IM::Login::IMLogoutReq> LogoutReqPtr;

typedef std::shared_ptr<IM::Server::IMValidateReq> IMValiReqPtr;
typedef std::shared_ptr<IM::Server::IMValidateRsp> ValidateRspPtr;
typedef std::shared_ptr<IM::Server::IMServerKickUser> ServerKickUserPtr;
typedef std::shared_ptr<IM::Server::IMServerPCLoginStatusNotify> PCLoginStatusNotifyPtr;

typedef std::shared_ptr<IM::Server::IMOnlineUserInfo> OnlineUserInfoPtr;
typedef std::shared_ptr<IM::Server::IMUserStatusUpdate> UserStatusUpdatePtr;
typedef std::shared_ptr<IM::Server::IMRoleSet> RoleSetPtr;

typedef std::shared_ptr<IM::Buddy::IMDepartmentReq> DepartmentReqPtr;
typedef std::shared_ptr<IM::Buddy::IMDepartmentRsp> DepartmentRspPtr;
typedef std::shared_ptr<IM::Buddy::IMAllUserReq> AllUserReqPtr;
typedef std::shared_ptr<IM::Buddy::IMAllUserRsp> AllUserRspPtr;
typedef std::shared_ptr<IM::Buddy::IMRecentContactSessionReq> RecentContactSessionReqPtr;
typedef std::shared_ptr<IM::Buddy::IMRecentContactSessionRsp> RecentContactSessionRspPtr;
typedef std::shared_ptr<IM::Buddy::IMUsersStatReq> UsersStatReqPtr;
typedef std::shared_ptr<IM::Buddy::IMUsersStatRsp> UsersStatRspPtr;
typedef std::shared_ptr<IM::Buddy::IMUserStatNotify> UserStatNotifyPtr;
typedef std::shared_ptr<IM::Buddy::IMRemoveSessionNotify> RemoveSessionNotifyPtr;
typedef std::shared_ptr<IM::Buddy::IMSignInfoChangedNotify> SignInfoChangedNotifyPtr;

typedef std::shared_ptr<IM::Group::IMNormalGroupListReq> NormalGroupListReqPtr;
typedef std::shared_ptr<IM::Group::IMNormalGroupListRsp> NormalGroupListRspPtr;
typedef std::shared_ptr<IM::Group::IMGroupChangeMemberRsp> GroupChangeMemberRspPtr;

typedef std::shared_ptr<IM::Message::IMMsgData> MsgDataPtr;
typedef std::shared_ptr<IM::Message::IMMsgDataReadNotify> MsgDataReadNotifyPtr;
typedef std::shared_ptr<IM::Message::IMUnreadMsgCntReq> UnreadMsgCntReqPtr;
typedef std::shared_ptr<IM::Message::IMUnreadMsgCntRsp> UnreadMsgCntRspPtr;
typedef std::shared_ptr<IM::Message::IMGetMsgListReq> GetMsgListReqPtr;
typedef std::shared_ptr<IM::Message::IMGetMsgListRsp> GetMsgListRspPtr;
typedef std::shared_ptr<IM::Message::IMMsgData> MsgDataPtr;

typedef std::shared_ptr<IM::File::IMFileHasOfflineReq> FileHasOfflineReqPtr;

typedef std::shared_ptr<IM::SwitchService::IMP2PCmdMsg> P2PCmdMsgPtr;