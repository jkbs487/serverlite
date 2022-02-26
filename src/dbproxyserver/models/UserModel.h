#include "DBPool.h"
#include "pbs/IM.BaseDefine.pb.h"

class UserModel
{
public:
    UserModel(DBPoolPtr dbPool);
    ~UserModel();

    void getChangedId(uint32_t& nLastTime, list<uint32_t>& lsIds);
    void getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo>& lsUsers);

private:
    DBPoolPtr dbPool_;
};
