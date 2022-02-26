#include "DBPool.h"
#include <string.h>

#define log_error printf
#define log_warn printf
#define log_info printf
#define MIN_DB_CONN_CNT 2
#define MAX_DB_CONN_FAIL_NUM 10


ResultSet::ResultSet(MYSQL_RES *res)
{
	res_ = res;

	// map table field key to index in the result array
	int num_fields = mysql_num_fields(res_);
	MYSQL_FIELD *fields = mysql_fetch_fields(res_);
	for (int i = 0; i < num_fields; i++)
	{
		// 多行
		keyMap_.insert(make_pair(fields[i].name, i));
	}
}

ResultSet::~ResultSet()
{
	if (res_)
	{
		mysql_free_result(res_);
		res_ = NULL;
	}
}

bool ResultSet::next()
{
	row_ = mysql_fetch_row(res_);
	if (row_) {
		return true;
	} else {
		return false;
	}
}

int ResultSet::_getIndex(const char *key)
{
	map<string, int>::iterator it = keyMap_.find(key);
	if (it == keyMap_.end()) {
		return -1;
	} else {
		return it->second;
	}
}

int ResultSet::getInt(const char *key)
{
	int idx = _getIndex(key);
	if (idx == -1) {
		return 0;
	} else {
		return atoi(row_[idx]); // 有索引
	}
}

char *ResultSet::getString(const char *key)
{
	int idx = _getIndex(key);
	if (idx == -1) {
		return NULL;
	} else {
		return row_[idx];		// 列
	}
}

/////////////////////////////////////////
PrepareStatement::PrepareStatement()
{
	stmt_ = NULL;
	paramBind_ = NULL;
	paramCnt_ = 0;
}

PrepareStatement::~PrepareStatement()
{
	if (stmt_)
	{
		mysql_stmt_close(stmt_);
		stmt_ = NULL;
	}

	if (paramBind_)
	{
		delete[] paramBind_;
		paramBind_ = NULL;
	}
}

bool PrepareStatement::init(MYSQL *mysql, string &sql)
{
	mysql_ping(mysql);	// 当mysql连接丢失的时候，使用mysql_ping能够自动重连数据库

	//g_master_conn_fail_num ++;
	stmt_ = mysql_stmt_init(mysql);
	if (!stmt_)
	{
		log_error("mysql_stmt_init failed\n");
		return false;
	}

	if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.size()))
	{
		log_error("mysql_stmt_prepare failed: %s\n", mysql_stmt_error(stmt_));
		return false;
	}

	paramCnt_ = static_cast<uint32_t>(mysql_stmt_param_count(stmt_));
	if (paramCnt_ > 0)
	{
		paramBind_ = new MYSQL_BIND[paramCnt_];
		if (!paramBind_)
		{
			log_error("new failed\n");
			return false;
		}

		memset(paramBind_, 0, sizeof(MYSQL_BIND) * paramCnt_);
	}

	return true;
}

void PrepareStatement::setParam(uint32_t index, int &value)
{
	if (index >= paramCnt_) {
		log_error("index too large: %d\n", index);
		return;
	}

	paramBind_[index].buffer_type = MYSQL_TYPE_LONG;
	paramBind_[index].buffer = &value;
}

void PrepareStatement::setParam(uint32_t index, uint32_t &value)
{
	if (index >= paramCnt_) {
		log_error("index too large: %d\n", index);
		return;
	}

	paramBind_[index].buffer_type = MYSQL_TYPE_LONG;
	paramBind_[index].buffer = &value;
}

void PrepareStatement::setParam(uint32_t index, string &value)
{
	if (index >= paramCnt_) {
		log_error("index too large: %d\n", index);
		return;
	}

	paramBind_[index].buffer_type = MYSQL_TYPE_STRING;
	paramBind_[index].buffer = const_cast<char*>(value.c_str());
	paramBind_[index].buffer_length = value.size();
}

void PrepareStatement::setParam(uint32_t index, const string &value)
{
	if (index >= paramCnt_) {
		log_error("index too large: %d\n", index);
		return;
	}

	paramBind_[index].buffer_type = MYSQL_TYPE_STRING;
	paramBind_[index].buffer =  const_cast<char*>(value.c_str());
	paramBind_[index].buffer_length = value.size();
}

bool PrepareStatement::executeUpdate()
{
	if (!stmt_)
	{
		log_error("no m_stmt\n");
		return false;
	}

	if (mysql_stmt_bind_param(stmt_, paramBind_))
	{
		log_error("mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(stmt_));
		return false;
	}

	if (mysql_stmt_execute(stmt_))
	{
		log_error("mysql_stmt_execute failed: %s\n", mysql_stmt_error(stmt_));
		return false;
	}

	if (mysql_stmt_affected_rows(stmt_) == 0)
	{
		log_error("ExecuteUpdate have no effect\n");
		return false;
	}

	return true;
}

uint32_t PrepareStatement::getInsertId()
{
	return static_cast<uint32_t>(mysql_stmt_insert_id(stmt_));
}

/////////////////////
DBConn::DBConn(DBPool *pool):
    pool_(pool),
    mysql_(nullptr)
{
}

DBConn::~DBConn()
{
	if (mysql_)
	{
		mysql_close(mysql_);
	}
}

int DBConn::init()
{
	mysql_ = mysql_init(NULL);	// mysql_标准的mysql c client对应的api
	if (!mysql_) {
		log_error("mysql_init failed\n");
		return 1;
	}

	my_bool reconnect = true;
	mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);	// 配合mysql_ping实现自动重连
	mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");	// utf8mb4和utf8区别

	// ip 端口 用户名 密码 数据库名
	if (!mysql_real_connect(mysql_, pool_->getDBServerIP().c_str(), pool_->getUsername().c_str(), pool_->getPasswrod().c_str(),
							pool_->getDBName().c_str(), pool_->getDBServerPort(), NULL, 0))
	{
		log_error("mysql_real_connect failed: %s\n", mysql_error(mysql_));
		return 2;
	}

	return 0;
}

const std::string& DBConn::getPoolName()
{
	return pool_->getPoolName();
}

bool DBConn::executeCreate(string sqlQuery)
{
	mysql_ping(mysql_);
	// mysql_real_query 实际就是执行了SQL
	if (mysql_real_query(mysql_, sqlQuery.data(), sqlQuery.size())) {
		log_error("mysql_real_query failed: %s, sql: start transaction\n", mysql_error(mysql_));
		return false;
	}

	return true;
}
bool DBConn::executeDrop(const char *sql_query)
{
	mysql_ping(mysql_);	// 如果端开了，能够自动重连

	if (mysql_real_query(mysql_, sql_query, strlen(sql_query))) {
		log_error("mysql_real_query failed: %s, sql: start transaction\n", mysql_error(mysql_));
		return false;
	}

	return true;
}

ResultSet *DBConn::executeQuery(string sqlQuery)
{
	mysql_ping(mysql_);

	if (mysql_real_query(mysql_, sqlQuery.data(), sqlQuery.size())) {
		log_error("mysql_real_query failed: %s, sql: %s\n", mysql_error(mysql_), sqlQuery.c_str());
		return NULL;
	}
	// 返回结果
	MYSQL_RES *res = mysql_store_result(mysql_);	// 返回结果
	if (!res)
	{
		log_error("mysql_store_result failed: %s\n", mysql_error(mysql_));
		return NULL;
	}

	ResultSet *resultSet = new ResultSet(res);	// 存储到CResultSet
	return resultSet;
}

/*
1.执行成功，则返回受影响的行的数目，如果最近一次查询失败的话，函数返回 -1

2.对于delete,将返回实际删除的行数.

3.对于update,如果更新的列值原值和新值一样,如update tables set col1=10 where id=1;
id=1该条记录原值就是10的话,则返回0。

mysql_affected_rows返回的是实际更新的行数,而不是匹配到的行数。
*/
bool DBConn::executeUpdate(const char *sql_query, bool care_affected_rows)
{
	mysql_ping(mysql_);

	if (mysql_real_query(mysql_, sql_query, strlen(sql_query))) {
		log_error("mysql_real_query failed: %s, sql: %s\n", mysql_error(mysql_), sql_query);
		//g_master_conn_fail_num ++;
		return false;
	}

	if (mysql_affected_rows(mysql_) > 0) {
		return true;
	} else { // 影响的行数为0时
		if (care_affected_rows) { // 如果在意影响的行数时, 返回false, 否则返回true
			log_error("mysql_real_query failed: %s, sql: %s\n\n", mysql_error(mysql_), sql_query);
			return false;
		} else {
			log_warn("affected_rows=0, sql: %s\n\n", sql_query);
			return true;
		}
	}
}

bool DBConn::startTransaction()
{
	mysql_ping(mysql_);

	if (mysql_real_query(mysql_, "start transaction\n", 17)) {
		log_error("mysql_real_query failed: %s, sql: start transaction\n", mysql_error(mysql_));
		return false;
	}

	return true;
}

bool DBConn::rollback()
{
	mysql_ping(mysql_);

	if (mysql_real_query(mysql_, "rollback\n", 8)) {
		log_error("mysql_real_query failed: %s, sql: rollback\n", mysql_error(mysql_));
		return false;
	}

	return true;
}

bool DBConn::commit()
{
	mysql_ping(mysql_);

	if (mysql_real_query(mysql_, "commit\n", 6)) {
		log_error("mysql_real_query failed: %s, sql: commit\n", mysql_error(mysql_));
		return false;
	}

	return true;
}
uint32_t DBConn::getInsertId()
{
	return static_cast<uint32_t>(mysql_insert_id(mysql_));
}

////////////////
DBPool::DBPool(const std::string& poolName, const std::string& dbServerIp, uint16_t dbServerPort,
				 const std::string& username, const std::string& password, const std::string& dbName, int maxConnCnt)
    : poolName_(poolName),
    dbServerIp_(dbServerIp),
    dbServerPort_(dbServerPort),
    username_(username),
    password_(password),
    dbName_(dbName),
    dbCurConnCnt_(MIN_DB_CONN_CNT), // 最小连接数量
	dbMaxConnCnt_(maxConnCnt)
{
}

// 释放连接池
DBPool::~DBPool()
{
	std::lock_guard<std::mutex> lock(mutex_);
	abortRequest_ = true;
	cond_.notify_all();		// 通知所有在等待的

	for (list<DBConn *>::iterator it = freeList_.begin(); it != freeList_.end(); it++) {
		DBConn *conn = *it;
		delete conn;
	}

	freeList_.clear();
}

int DBPool::init()
{
	// 创建固定最小的连接数量
	for (int i = 0; i < dbCurConnCnt_; i++)
	{
		DBConn *conn = new DBConn(this);
		int ret = conn->init();
		if (ret) {
			delete conn;
			return ret;
		}

		freeList_.push_back(conn);
	}

	// log_error("db pool: %s, size: %d\n", m_pool_name.c_str(), (int)m_free_list.size());
	return 0;
}

/*
 *TODO: 增加保护机制，把分配的连接加入另一个队列，这样获取连接时，如果没有空闲连接，
 *TODO: 检查已经分配的连接多久没有返回，如果超过一定时间，则自动收回连接，放在用户忘了调用释放连接的接口
 * timeout_ms默认为-1死等
 * timeout_ms >=0 则为等待的时间
 */
int wait_cout = 0;
DBConn *DBPool::getDBConn(const int timeout_ms)
{
	std::unique_lock<std::mutex> lock(mutex_);
	if(abortRequest_) {
		log_warn("have aboort\n");
		return NULL;
	}

    // 当没有连接可以用时
	if (freeList_.empty()) {
		// 第一步先检测 当前连接数量是否达到最大的连接数量 
		if (dbCurConnCnt_ >= dbMaxConnCnt_)
		{
			// 如果已经到达了，看看是否需要超时等待
			// 死等，直到有连接可以用 或者 连接池要退出
            if(timeout_ms < 0) {
				log_info("wait ms:%d\n", timeout_ms);
				cond_.wait(lock, [this] {
					// log_info("wait:%d, size:%d\n", wait_cout++, m_free_list.size());
					// 当前连接数量小于最大连接数量 或者请求释放连接池时退出
					return (!freeList_.empty()) | abortRequest_;
				});
			} else {
				// return如果返回 false，继续wait(或者超时),  如果返回true退出wait
				// 1.m_free_list不为空
				// 2.超时退出
				// 3. m_abort_request被置为true，要释放整个连接池
				cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] {
					// log_info("wait_for:%d, size:%d\n", wait_cout++, m_free_list.size());
					return (!freeList_.empty()) | abortRequest_;
				});
				// 带超时功能时还要判断是否为空
                // 如果连接池还是没有空闲则退出
				if(freeList_.empty()) {
					return NULL;
				}
			}

			if(abortRequest_) {
				log_warn("have aboort\n");
				return NULL;
			}
		}
        // 还没有到最大连接则创建连接
		else {
			DBConn *conn = new DBConn(this);	//新建连接
			int ret = conn->init();
			if (ret) {
				log_error("Init DBConnecton failed\n\n");
				delete conn;
				return NULL;
			} else {
				freeList_.push_back(conn);
				dbCurConnCnt_++;
				log_info("new db connection: %s, conn_cnt: %d\n", poolName_.c_str(), dbCurConnCnt_);
			}
		}
	}

	DBConn *conn = freeList_.front();	// 获取连接
	freeList_.pop_front();	// STL 吐出连接，从空闲队列删除
	// pConn->setCurrentTime();  // 伪代码
	usedList_.push_back(conn);		// 

	return conn;
}

void DBPool::relDBConn(DBConn *conn)
{
	std::lock_guard<std::mutex> lock(mutex_);

	list<DBConn *>::iterator it = freeList_.begin();
    // 避免重复归还
	for (; it != freeList_.end(); it++) {
		if (*it == conn) {
			break;
		}
	}

	if (it == freeList_.end()) {
		usedList_.remove(conn);
		freeList_.push_back(conn);
		cond_.notify_one();		// 通知取队列
	} else {
		log_error("RelDBConn failed\n");
	}
}
// 遍历检测是否超时未归还
// pConn->isTimeout(); // 当前时间 - 被请求的时间
// 强制回收  从m_used_list 放回 m_free_list