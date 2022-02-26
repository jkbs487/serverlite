#include "slite/TCPServer.h"
#include "slite/TCPClient.h"
#include "slite/EventLoop.h"
#include "slite/ThreadPool.h"
#include "slite/Logger.h"

#include "pbs/IM.Login.pb.h"
#include "pbs/IM.Server.pb.h"
#include "pbs/IM.Buddy.pb.h"
#include "pbs/IM.Group.pb.h"
#include "pbs/IM.Other.pb.h"

#include "EncDec.h"
#include "models/DBPool.h"
#include "models/DepartmentModel.h"
#include "models/UserModel.h"

#include "slite/protobuf/codec.h"
#include "slite/protobuf/dispatcher.h"

#include <set>
#include <memory>
#include <functional>
#include <sys/time.h>

using namespace slite;
using namespace std::placeholders;

typedef std::shared_ptr<IM::Server::IMValidateReq> IMValiReqPtr;
typedef std::shared_ptr<IM::Login::IMLogoutReq> LogoutReqPtr;
typedef std::shared_ptr<IM::Buddy::IMDepartmentReq> DepartmentReqPtr;
typedef std::shared_ptr<IM::Buddy::IMAllUserReq> AllUserReqPtr;
typedef std::shared_ptr<IM::Buddy::IMRecentContactSessionReq> RecentContactSessionReqPtr;
typedef std::shared_ptr<IM::Buddy::IMUsersStatReq> UsersStatReqPtr;

typedef std::shared_ptr<IM::Group::IMNormalGroupListReq> NormalGroupListReqPtr;
typedef std::shared_ptr<IM::Group::IMGroupChangeMemberRsp> GroupChangeMemberRspPtr;

class DBProxyServer
{
public:
    DBProxyServer(std::string host, uint16_t port, EventLoop* loop);
    ~DBProxyServer();

    void start() {
        threadPool_.start(4);
        server_.start();
    }
    void onTimer();

private:
    DBProxyServer(const DBProxyServer& server) = delete;

    struct Context {
        int64_t lastRecvTick;
        int64_t lastSendTick;
    };

    void onConnection(const TCPConnectionPtr& conn);
    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);
    void onWriteComplete(const TCPConnectionPtr& conn);
    void onUnknownMessage(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onHeartBeat(const TCPConnectionPtr& conn, const MessagePtr& message, int64_t receiveTime);
    void onValidateRequest(const TCPConnectionPtr& conn, const IMValiReqPtr& message, int64_t receiveTime);

    void onClientDepartmentRequest(const TCPConnectionPtr& conn, const DepartmentReqPtr& message, int64_t receiveTime);
    void onClientAllUserRequest(const TCPConnectionPtr& conn, const AllUserReqPtr& message, int64_t receiveTime);

    bool doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo& user);

    static const int kHeartBeatInterVal = 5000;
    static const int kTimeout = 30000;

    TCPServer server_;
    EventLoop *loop_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
    ThreadPool threadPool_;
    DBPoolPtr dbPool_;
    std::unique_ptr<DepartmentModel> departModel_;
    std::unique_ptr<UserModel> userModel_;
    std::set<TCPConnectionPtr> clientConns_;
    std::map<string, std::list<uint32_t> > hmLimits_;
};

DBProxyServer::DBProxyServer(std::string host, uint16_t port, EventLoop* loop):
    server_(host, port, loop, "MsgServer"),
    loop_(loop),
    dispatcher_(std::bind(&DBProxyServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3)),
    threadPool_("DBThreadPool"),
    dbPool_(new DBPool("IMDBPool", "127.0.0.1", 3306, "root", "jk111111", "teamtalk", 5)),
    departModel_(new DepartmentModel(dbPool_)),
    userModel_(new UserModel(dbPool_))
{
    server_.setConnectionCallback(
        std::bind(&DBProxyServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&DBProxyServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(
        std::bind(&DBProxyServer::onWriteComplete, this, _1));
    dispatcher_.registerMessageCallback<IM::Other::IMHeartBeat>(
        std::bind(&DBProxyServer::onHeartBeat, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Server::IMValidateReq>(
        std::bind(&DBProxyServer::onValidateRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMDepartmentReq>(
        std::bind(&DBProxyServer::onClientDepartmentRequest, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<IM::Buddy::IMAllUserReq>(
        std::bind(&DBProxyServer::onClientAllUserRequest, this, _1, _2, _3));

    loop_->runEvery(1.0, std::bind(&DBProxyServer::onTimer, this));
    threadPool_.setMaxQueueSize(10);
}

DBProxyServer::~DBProxyServer()
{
    threadPool_.stop();
}

void DBProxyServer::onTimer()
{
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    int64_t currTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;

    for (auto conn : clientConns_) {
        Context* context = std::any_cast<Context*>(conn->getContext());

        if (currTick > context->lastSendTick + kHeartBeatInterVal) {
            IM::Other::IMHeartBeat msg;
            codec_.send(conn, msg);
        }
        
        if (currTick > context->lastRecvTick + kTimeout) {
            LOG_ERROR << "Connect to MsgServer timeout";
            conn->forceClose();
        }
    }
}

void DBProxyServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        Context* context = new Context();
        struct timeval tval;
        ::gettimeofday(&tval, NULL);
        context->lastRecvTick = context->lastSendTick = 
            tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        conn->setContext(context);
        clientConns_.insert(conn);
    } else {
        clientConns_.erase(conn);
    }
}

void DBProxyServer::onMessage(const TCPConnectionPtr& conn, 
                            std::string& buffer, 
                            int64_t receiveTime)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
    std::string buf;
    buf.swap(buffer);
    threadPool_.run(
        std::bind(&ProtobufCodec::onMessage, &codec_, conn, buf, receiveTime));
}

void DBProxyServer::onWriteComplete(const TCPConnectionPtr& conn)
{
    Context* context = std::any_cast<Context*>(conn->getContext());
    struct timeval tval;
    ::gettimeofday(&tval, NULL);
    context->lastSendTick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
}

void DBProxyServer::onUnknownMessage(const TCPConnectionPtr& conn,
                                const MessagePtr& message,
                                int64_t receiveTime)
{
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
}

void DBProxyServer::onHeartBeat(const TCPConnectionPtr& conn,
                            const MessagePtr& message,
                            int64_t receiveTime)
{
    LOG_INFO << "onHeartBeat: " << message->GetTypeName();
    Context* context = std::any_cast<Context*>(conn->getContext());
    context->lastRecvTick = receiveTime;
}

bool DBProxyServer::doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo& user)
{
    bool ret = false;
    DBConn* dbConn = dbPool_->getDBConn();
    if (dbConn) {
        string strSql = "SELECT * FROM IMUser WHERE name='" + strName + "' AND status=0";
        ResultSet* resultSet = dbConn->executeQuery(strSql);
        if(resultSet) {
            string strResult, strSalt;
            uint32_t nId, nGender, nDeptId, nStatus;
            string strNick, strAvatar, strEmail, strRealName, strTel, strDomain,strSignInfo;
            while (resultSet->next()) {
                nId = resultSet->getInt("id");
                strResult = resultSet->getString("password");
                strSalt = resultSet->getString("salt");
                
                strNick = resultSet->getString("nick");
                nGender = resultSet->getInt("sex");
                strRealName = resultSet->getString("name");
                strDomain = resultSet->getString("domain");
                strTel = resultSet->getString("phone");
                strEmail = resultSet->getString("email");
                strAvatar = resultSet->getString("avatar");
                nDeptId = resultSet->getInt("departId");
                nStatus = resultSet->getInt("status");
                strSignInfo = resultSet->getString("sign_info");
            }

            string strInPass = strPass + strSalt;
            char szMd5[33];
            Md5::MD5_Calculate(strInPass.c_str(), static_cast<u_int32_t>(strInPass.length()), szMd5);
            string strOutPass(szMd5);
            if(strOutPass == strResult)
            {
                ret = true;
                user.set_user_id(nId);
                user.set_user_nick_name(strNick);
                user.set_user_gender(nGender);
                user.set_user_real_name(strRealName);
                user.set_user_domain(strDomain);
                user.set_user_tel(strTel);
                user.set_email(strEmail);
                user.set_avatar_url(strAvatar);
                user.set_department_id(nDeptId);
                user.set_status(nStatus);
  	            user.set_sign_info(strSignInfo);
            }
            delete resultSet;
        }
        dbPool_->relDBConn(dbConn);
    }
    return ret;
}

void DBProxyServer::onValidateRequest(const TCPConnectionPtr& conn, 
                        const IMValiReqPtr& message, 
                        int64_t receiveTime)
{
    IM::Server::IMValidateRsp resp;
    std::string strDomain = message->user_name();
    std::string strPass = message->password();

    resp.set_user_name(strDomain);
    resp.set_attach_data(message->attach_data());

    do {
        // lock
        std::list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        uint32_t tmNow = static_cast<uint32_t>(time(NULL));

        auto itTime = lsErrorTime.begin();
        for (; itTime != lsErrorTime.end(); ++itTime) {
            if (tmNow - *itTime > 30 * 60) {
                break;
            }
        }
        if (itTime != lsErrorTime.end()) {
            lsErrorTime.erase(itTime);
        }

        if (lsErrorTime.size() > 10) {
            itTime = lsErrorTime.begin();
            if (tmNow - *itTime <= 30 * 60) {
                resp.set_result_code(6);
                resp.set_result_string("用户名/密码错误次数太多");
                codec_.send(conn, resp);
                return;
            }
        }
    } while (false);

    LOG_INFO << strDomain << " request login";

    IM::BaseDefine::UserInfo user;
    if (doLogin(strDomain, strPass, user)) {
        IM::BaseDefine::UserInfo* pUser = resp.mutable_user_info();
        pUser->set_user_id(user.user_id());
        pUser->set_user_gender(user.user_gender());
        pUser->set_department_id(user.department_id());
        pUser->set_user_nick_name(user.user_nick_name());
        pUser->set_user_domain(user.user_domain());
        pUser->set_avatar_url(user.avatar_url());
        
        pUser->set_email(user.email());
        pUser->set_user_tel(user.user_tel());
        pUser->set_user_real_name(user.user_real_name());
        pUser->set_status(0);

        pUser->set_sign_info(user.sign_info());
        
        resp.set_result_code(0);
        resp.set_result_string("成功");
        
        //如果登陆成功，则清除错误尝试限制
        // lock
        list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        lsErrorTime.clear();
    } else {
        //密码错误，记录一次登陆失败
        uint32_t tmCurrent =  static_cast<uint32_t>(time(NULL));
        // lock
        list<uint32_t>& lsErrorTime = hmLimits_[strDomain];
        lsErrorTime.push_front(tmCurrent);
        
        LOG_WARN << "get result false";
        resp.set_result_code(1);
        resp.set_result_string("用户名/密码错误");
    }

    codec_.send(conn, resp);
}

void DBProxyServer::onClientDepartmentRequest(const TCPConnectionPtr& conn, 
                                            const DepartmentReqPtr& message, 
                                            int64_t receiveTime)
{
    IM::Buddy::IMDepartmentRsp resp;
    uint32_t userId = message->user_id();
    uint32_t lastUpdate = message->latest_update_time();
    std::list<uint32_t> lsChangeIds;
    departModel_->getChgedDeptId(lastUpdate, lsChangeIds);
    std::list<IM::BaseDefine::DepartInfo> lsDeparts;
    departModel_->getDepts(lsChangeIds, lsDeparts);

    resp.set_user_id(userId);
    resp.set_latest_update_time(lastUpdate);
    for (auto lsDepart : lsDeparts) {
        IM::BaseDefine::DepartInfo* pDeptInfo = resp.add_dept_list();
        pDeptInfo->set_dept_id(lsDepart.dept_id());
        pDeptInfo->set_priority(lsDepart.priority());
        pDeptInfo->set_dept_name(lsDepart.dept_name());
        pDeptInfo->set_parent_dept_id(lsDepart.parent_dept_id());
        pDeptInfo->set_dept_status(lsDepart.dept_status());
    }
    LOG_INFO << "userId=" << userId << ", lastUpdate=" 
        << lastUpdate << ", cnt=" << lsDeparts.size();
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

void DBProxyServer::onClientAllUserRequest(const TCPConnectionPtr& conn, 
                                        const AllUserReqPtr& message, 
                                        int64_t receiveTime)
{
    IM::Buddy::IMAllUserRsp resp;
    uint32_t nReqId = message->user_id();
    uint32_t nLastTime = message->latest_update_time();
    uint32_t nLastUpdate = static_cast<uint32_t>(time(NULL)); //CSyncCenter::getInstance()->getLastUpdate();
    
    list<IM::BaseDefine::UserInfo> lsUsers;
    if( nLastUpdate > nLastTime)
    {
        list<uint32_t> lsIds;
        userModel_->getChangedId(nLastTime, lsIds);
        userModel_->getUsers(lsIds, lsUsers);
    }
    resp.set_user_id(nReqId);
    resp.set_latest_update_time(nLastTime);
    for (auto lsUser : lsUsers) {
        IM::BaseDefine::UserInfo* pUser = resp.add_user_list();
        pUser->set_user_id(lsUser.user_id());
        pUser->set_user_gender(lsUser.user_gender());
        pUser->set_user_nick_name(lsUser.user_nick_name());
        pUser->set_avatar_url(lsUser.avatar_url());
        pUser->set_sign_info(lsUser.sign_info());
        pUser->set_department_id(lsUser.department_id());
        pUser->set_email(lsUser.email());
        pUser->set_user_real_name(lsUser.user_real_name());
        pUser->set_user_tel(lsUser.user_tel());
        pUser->set_user_domain(lsUser.user_domain());
        pUser->set_status(lsUser.status());
    }

    LOG_INFO << "userId=" << nReqId << ", nLastUpdate=" << nLastUpdate 
        << ", last_time=" << nLastTime << ", userCnt=" << resp.user_list_size();
    resp.set_attach_data(message->attach_data());
    codec_.send(conn, resp);
}

int main()
{
    Logger::setLogLevel(Logger::DEBUG);

    EventLoop loop;
    DBProxyServer dbProxyServer("0.0.0.0", 10003, &loop);
    dbProxyServer.start();
    loop.loop();
}