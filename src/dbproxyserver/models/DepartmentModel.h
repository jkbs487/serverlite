#include <list>

#include "DBPool.h"
#include "pbs/IM.BaseDefine.pb.h"

class DepartmentModel
{
public:
    DepartmentModel(DBPoolPtr dbPool);
    ~DepartmentModel();
    void getChgedDeptId(uint32_t& nLastTime, list<uint32_t>& lsChangedIds);
    void getDepts(list<uint32_t>& lsDeptIds, list<IM::BaseDefine::DepartInfo>& lsDepts);
    void getDept(uint32_t nDeptId, IM::BaseDefine::DepartInfo& cDept);
private:
    DBPoolPtr dbPool_;
};