#pragma once

#include <iostream>
#include <list>
#include <mutex>
#include <condition_variable>
#include <map>
#include <stdint.h>

#include <mysql/mysql.h>

#define MAX_ESCAPE_STRING_LEN	10240

using namespace std;

// 返回结果 select的时候用
class ResultSet {
public:
	ResultSet(MYSQL_RES* res);
	virtual ~ResultSet();

	bool next();
	int getInt(const char* key);
	char* getString(const char* key);
private:
	int _getIndex(const char* key);

	MYSQL_RES* res_;
	MYSQL_ROW row_;
	map<string, int> keyMap_;
};

// 插入数据用
class PrepareStatement {
public:
	PrepareStatement();
	virtual ~PrepareStatement();

	bool init(MYSQL* mysql, string& sql);

	void setParam(uint32_t index, int& value);
	void setParam(uint32_t index, uint32_t& value);
    void setParam(uint32_t index, string& value);
    void setParam(uint32_t index, const string& value);

	bool executeUpdate();
	uint32_t getInsertId();
private:
	MYSQL_STMT*	stmt_;
	MYSQL_BIND*	paramBind_;
	uint32_t paramCnt_;
};

class DBPool;

class DBConn {
public:
	DBConn(DBPool* pool);
	virtual ~DBConn();
	int init();

	// 创建表
	bool executeCreate(string sql_query);
	// 删除表
	bool executeDrop(const char* sql_query);
	// 查询
	ResultSet* executeQuery(string sql_query);

    /**
    *  执行DB更新，修改
    *
    *  @param sql_query     sql
    *  @param care_affected_rows  是否在意影响的行数，false:不在意；true:在意
    *
    *  @return 成功返回true 失败返回false
    */
	bool executeUpdate(std::string sqlQuery, bool careAffectedRows = true);
	uint32_t getInsertId();

	// 开启事务
	bool startTransaction();
	// 提交事务
	bool commit();
	// 回滚事务
	bool rollback();
	// 获取连接池名
	const std::string& getPoolName();
	MYSQL* getMysql() { return mysql_; }
private:
	DBPool* pool_;	// to get MySQL server information
	MYSQL* mysql_;	// 对应一个连接
	char escapeString[MAX_ESCAPE_STRING_LEN + 1];
};

class DBPool {	// 只是负责管理连接CDBConn，真正干活的是CDBConn
public:
	DBPool(const std::string& poolName, const std::string& dbServerIp, uint16_t dbServerPort,
			const std::string& username, const std::string& password, const std::string& dbName, 
			int maxConnCnt);
	virtual ~DBPool();

	int init();		// 连接数据库，创建连接
	DBConn* getDBConn(const int timeout_ms = -1);	// 获取连接资源
	void relDBConn(DBConn* pConn);	// 归还连接资源

	const std::string& getPoolName() const
	{ return poolName_; }
	const std::string& getDBServerIP() const 
	{ return dbServerIp_; }
	uint16_t getDBServerPort() const
	{ return dbServerPort_; }
	const std::string& getUsername() const
	{ return username_; }
	const std::string& getPasswrod() const
	{ return password_; }
	const std::string& getDBName() const
	{ return dbName_; }
private:
	string poolName_;	// 连接池名称
	string dbServerIp_;	// 数据库ip
	uint16_t dbServerPort_; // 数据库端口
	string username_;  	// 用户名
	string password_;		// 用户密码
	string dbName_;		// db名称
	int	dbCurConnCnt_;	// 当前启用的连接数量
	int dbMaxConnCnt_;	// 最大连接数量
	std::list<DBConn*> freeList_;	// 空闲的连接
	std::list<DBConn*> usedList_;		// 记录已经被请求的连接
	std::mutex mutex_;
    std::condition_variable cond_;
	bool abortRequest_ = false;
	// CThreadNotify	m_free_notify;	// 信号量
};

typedef std::shared_ptr<DBPool> DBPoolPtr;