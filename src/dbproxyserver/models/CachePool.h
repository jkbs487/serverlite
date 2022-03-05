/*
 * @Author: your name
 * @Date: 2019-12-07 10:54:57
 * @LastEditTime : 2020-01-10 16:35:13
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \src\cache_pool\CachePool.h
 */
#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include "hiredis.h"

using std::string;
using std::list;
using std::map; 
using std::vector; 

class CachePool;

class CacheConn {
public:
	CacheConn(const string& serverIp, uint16_t serverPort, uint16_t dbIndex, const string& password,
			const string& poolName);
	CacheConn(CachePool* pCachePool);	
	virtual ~CacheConn();
	
	int init();
	void deInit();
	string getPoolName();
    // 通用操作
    // 判断一个key是否存在
    bool isExists(string &key);
    // 删除某个key
    long del(string &key);

    // ------------------- 字符串相关 -------------------
	string get(string key);
    string set(string key, string& value);
	string setex(string key, int timeout, string value);
	
	// string mset(string key, map);
    //批量获取
    bool mget(const vector<string>& keys, map<string, string>& ret_value);
	//原子加减1
    long incr(string key);
    long decr(string key);

	// ---------------- 哈希相关 ------------------------
	long hdel(string key, string field);
	string hget(string key, string field);
	bool hgetAll(string key, map<string, string>& ret_value);
	long hset(string key, string field, string value);

	long hincrBy(string key, string field, long value);
    long incrBy(string key, long value);
	string hmset(string key, map<string, string>& hash);
	bool hmget(string key, list<string>& fields, list<string>& ret_value);

	// ------------ 链表相关 ------------
	long lpush(string key, string value);
	long rpush(string key, string value);
	long llen(string key);
	bool lrange(string key, long start, long end, list<string>& ret_value);

    bool flushdb();

private:
	CachePool* cachePool_;
	redisContext* context_;
	string serverIp_;
	uint16_t serverPort_;
    string password_;
	uint16_t dbIndex_;
	string poolName_;
	uint64_t lastConnectTime_;
};


class CachePool {
public:
	// db_index和mysql不同的地方 
	CachePool(string poolName, string serverIp, uint16_t serverPort, uint16_t dbIndex, 
		string password, int maxConnCnt);
	virtual ~CachePool();

	int init();
    // 获取空闲的连接资源
	CacheConn* getCacheConn();
    // Pool回收连接资源
	void relCacheConn(CacheConn* pCacheConn);

	const string& getPoolName() { return poolName_; }
	const string& getServerIP() { return serverIp_; }
	const string& getPassword() { return password_; }
	uint16_t getServerPort() { return serverPort_; }
	uint16_t getDBIndex() { return dbIndex_; }
private:
	string poolName_;
	string serverIp_;
	string password_;
	uint16_t serverPort_;
	uint16_t dbIndex_;	// mysql 数据库名字， redis db index

	int maxConnCnt_;
	int curConnCnt_;
	list<CacheConn*> freeList_;
	std::condition_variable cond_;
	std::mutex mutex_;
	//CThreadNotify freeNotify_;
};

typedef std::shared_ptr<CachePool> CachePoolPtr;
