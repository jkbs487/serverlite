#include "CachePool.h"

#include "slite/Logger.h"

#include <stdlib.h>
#include <cstring>

#define log_error printf
#define log_info printf

#define MIN_CACHE_CONN_CNT 2
#define MAX_CACHE_CONN_FAIL_NUM 10

using namespace slite;

CacheConn::CacheConn(const string& serverIp, uint16_t serverPort, uint16_t dbIndex, const string& password,
					 const string& poolName)
	: context_(nullptr),
	serverIp_(serverIp),
	serverPort_(serverPort),
	password_(password),
	dbIndex_(dbIndex),
	poolName_(poolName),
	lastConnectTime_(0)
{
}

CacheConn::CacheConn(CachePool *cachePool)
	: cachePool_(cachePool),
	context_(nullptr),
	lastConnectTime_(0)
{
	if (cachePool) {
		serverIp_ = cachePool->getServerIP();
		serverPort_ = cachePool->getServerPort();
		dbIndex_ = cachePool->getDBIndex();
		password_ = cachePool->getPassword();
		poolName_ = cachePool->getPoolName();
	} else {
		log_error("cachePool is NULL\n");
	}
}

CacheConn::~CacheConn()
{
	if (context_) {
		redisFree(context_);
		context_ = nullptr;
	}
}

/*
 * redis初始化连接和重连操作，类似mysql_ping()
 */
int CacheConn::init()
{
	if (context_) {	// 非空，连接是正常的
		return 0;
	}

	// 1s 尝试重连一次
	uint64_t curTime = static_cast<uint64_t>(time(NULL));
	if (curTime < lastConnectTime_ + 1) {		// 重连尝试 间隔1秒 
		printf("curTime:%lu, m_last_connect_time:%lu\n", curTime, lastConnectTime_);
		return 1;
	}
	// printf("m_last_connect_time = curTime\n");
	lastConnectTime_ = curTime;

	// 1000ms超时
	struct timeval timeout = {0, 1000000};
	// 建立连接后使用 redisContext 来保存连接状态。
	// redisContext 在每次操作后会修改其中的 err 和  errstr 字段来表示发生的错误码（大于0）和对应的描述。
	context_ = redisConnectWithTimeout(serverIp_.c_str(), serverPort_, timeout);
	if (!context_ || context_->err) {
		if (context_) {
			LOG_ERROR << "redisConnect failed: " << context_->errstr;
			redisFree(context_);
			context_ = nullptr;
		} else {
			LOG_ERROR << "redisConnect failed";
		}

		return 1;
	}

	redisReply *reply;
	// 验证
	if (!password_.empty()) {
		reply = static_cast<redisReply*>(redisCommand(context_, "AUTH %s", password_.c_str()));

		if (!reply || reply->type == REDIS_REPLY_ERROR) {
			log_error("Authentication failure:%p\n", reply);
			if (reply)
				freeReplyObject(reply);
			return -1;
		} else {
			log_info("Authentication success\n");
		}

		freeReplyObject(reply);
	}

	reply = static_cast<redisReply*>(redisCommand(context_, "SELECT %d", 0));

	if (reply && (reply->type == REDIS_REPLY_STATUS) && (!strncmp(reply->str, "OK", 2))) {
		freeReplyObject(reply);
		return 0;
	} else {
		if (reply)
			LOG_ERROR << "select cache db failed: " << reply->str;
		return 2;
	}
}

void CacheConn::deInit()
{
	if (context_) {
		redisFree(context_);
		context_ = NULL;
	}
}

string CacheConn::getPoolName()
{
	return poolName_;
}

string CacheConn::get(string key)
{
	string value;

	if (init())
		return value;

	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, "GET %s", key.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return value;
	}

	if (reply->type == REDIS_REPLY_STRING) {
		value.append(reply->str, reply->len);
	}

	freeReplyObject(reply);
	return value;
}

string CacheConn::set(string key, string &value)
{
	string retValue;

	if (init()) 
		return retValue;
		
	// 返回的结果存放在redisReply
	redisReply *reply = static_cast<redisReply*>(
		redisCommand(context_, "SET %s %s", key.c_str(), value.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return retValue;
	}

	retValue.append(reply->str, reply->len);
	freeReplyObject(reply); // 释放资源
	return retValue;
}

string CacheConn::setex(string key, int timeout, string value)
{
	string retValue;

	if (init())
		return retValue;

	redisReply *reply = static_cast<redisReply*>(
		redisCommand(context_, "SETEX %s %d %s", key.c_str(), timeout, value.c_str()));
	if (!reply)
	{
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return retValue;
	}

	retValue.append(reply->str, reply->len);
	freeReplyObject(reply);
	return retValue;
}

bool CacheConn::mget(const vector<string> &keys, map<string, string> &ret_value)
{
	if (init())
		return false;

	if (keys.empty())
		return false;

	string strKey;
	bool bFirst = true;
	for (const auto& key : keys) {
		if (bFirst) {
			bFirst = false;
			strKey = key;
		} else {
			strKey += " " + key;
		}
	}

	if (strKey.empty())
		return false;

	strKey = "MGET " + strKey;
	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, strKey.c_str()));
	if (!reply) {
		log_info("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return false;
	}
	if (reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; ++i) {
			redisReply *child_reply = reply->element[i];
			if (child_reply->type == REDIS_REPLY_STRING) {
				ret_value[keys[i]] = child_reply->str;
			}
		}
	}
	freeReplyObject(reply);
	return true;
}

bool CacheConn::isExists(string &key)
{
	if (init())
		return false;

	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, "EXISTS %s", key.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return false;
	}
	long ret_value = reply->integer;
	freeReplyObject(reply);
	
	if (0 == ret_value)
		return false;
	else
		return true;
}

long CacheConn::del(string &key)
{
	if (init())
		return 0;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "DEL %s", key.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return 0;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::hdel(string key, string field)
{
	if (init())
		return 0;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "HDEL %s %s", key.c_str(), field.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return 0;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

string CacheConn::hget(string key, string field)
{
	string ret_value;
	if (init())
		return ret_value;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "HGET %s %s", key.c_str(), field.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return ret_value;
	}

	if (reply->type == REDIS_REPLY_STRING) {
		ret_value.append(reply->str, reply->len);
	}

	freeReplyObject(reply);
	return ret_value;
}

bool CacheConn::hgetAll(string key, map<string, string> &ret_value)
{
	if (init())
		return false;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "HGETALL %s", key.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return false;
	}

	if ((reply->type == REDIS_REPLY_ARRAY) && (reply->elements % 2 == 0)) {
		for (size_t i = 0; i < reply->elements; i += 2) {
			redisReply *field_reply = reply->element[i];
			redisReply *value_reply = reply->element[i + 1];

			string field(field_reply->str, field_reply->len);
			string value(value_reply->str, value_reply->len);
			ret_value.insert(make_pair(field, value));
		}
	}

	freeReplyObject(reply);
	return true;
}

long CacheConn::hset(string key, string field, string value)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return -1;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::hincrBy(string key, string field, long value)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "HINCRBY %s %s %ld", key.c_str(), field.c_str(), value));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return -1;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::incrBy(string key, long value)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "INCRBY %s %ld", key.c_str(), value));
	if (!reply) {
		log_error("redis Command failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return -1;
	}
	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

string CacheConn::hmset(string key, map<string, string> &hash)
{
	string ret_value;

	if (init())
		return ret_value;

	int argc = static_cast<int>(hash.size()) * 2 + 2;
	const char **argv = new const char *[argc];
	if (!argv)
		return ret_value;

	argv[0] = "HMSET";
	argv[1] = key.c_str();
	int i = 2;
	for (const auto& h : hash) {
		argv[i++] = h.first.c_str();
		argv[i++] = h.second.c_str();
	}

	redisReply *reply = static_cast<redisReply*>(redisCommandArgv(context_, argc, argv, NULL));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		delete[] argv;

		redisFree(context_);
		context_ = NULL;
		return ret_value;
	}

	ret_value.append(reply->str, reply->len);

	delete[] argv;
	freeReplyObject(reply);
	return ret_value;
}

bool CacheConn::hmget(string key, list<string> &fields, list<string> &ret_value)
{
	if (init())
		return false;

	int argc = static_cast<int>(fields.size()) + 2;
	const char **argv = new const char *[argc];
	if (!argv)
		return false;

	argv[0] = "HMGET";
	argv[1] = key.c_str();
	int index = 2;

	for (const auto& field : fields) {
		argv[index++] = field.c_str();
	}

	redisReply *reply = static_cast<redisReply*>(redisCommandArgv(
		context_, argc, const_cast<const char **>(argv), NULL));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		delete[] argv;

		redisFree(context_);
		context_ = NULL;

		return false;
	}

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; i++) {
			redisReply *value_reply = reply->element[i];
			string value(value_reply->str, value_reply->len);
			ret_value.push_back(value);
		}
	}

	delete[] argv;
	freeReplyObject(reply);
	return true;
}

long CacheConn::incr(string key)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, "INCR %s", key.c_str()));
	if (!reply) {
		log_error("redis Command failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return -1;
	}
	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::decr(string key)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, "DECR %s", key.c_str()));
	if (!reply) {
		log_error("redis Command failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return -1;
	}
	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::lpush(string key, string value)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "LPUSH %s %s", key.c_str(), value.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return -1;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::rpush(string key, string value)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "RPUSH %s %s", key.c_str(), value.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return -1;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

long CacheConn::llen(string key)
{
	if (init())
		return -1;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "LLEN %s", key.c_str()));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return -1;
	}

	long ret_value = reply->integer;
	freeReplyObject(reply);
	return ret_value;
}

bool CacheConn::lrange(string key, long start, long end, list<string> &ret_value)
{
	if (init())
		return false;

	redisReply *reply = static_cast<redisReply*>(redisCommand(
		context_, "LRANGE %s %d %d", key.c_str(), start, end));
	if (!reply)
	{
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = NULL;
		return false;
	}

	if (reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; i++) {
			redisReply *value_reply = reply->element[i];
			string value(value_reply->str, value_reply->len);
			ret_value.push_back(value);
		}
	}

	freeReplyObject(reply);
	return true;
}

bool CacheConn::flushdb()
{
	bool ret = false;
	if (init())
		return false;

	redisReply *reply = static_cast<redisReply*>(redisCommand(context_, "FLUSHDB"));
	if (!reply) {
		log_error("redisCommand failed:%s\n", context_->errstr);
		redisFree(context_);
		context_ = nullptr;
		return false;
	}

	if (reply->type == REDIS_REPLY_STRING && strncmp(reply->str, "OK", 2) == 0) {
		ret = true;
	}

	freeReplyObject(reply);

	return ret;
}
///////////////
CachePool::CachePool(string poolName, string serverIp, uint16_t serverPort, uint16_t dbIndex,
					 string password, int maxConnCnt)
	: poolName_(poolName),
	serverIp_(serverIp),
	password_(password),
	serverPort_(serverPort),
	dbIndex_(dbIndex),
	maxConnCnt_(maxConnCnt),
	curConnCnt_(MIN_CACHE_CONN_CNT)
{
}

CachePool::~CachePool()
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto conn: freeList_) {
		CacheConn *pConn = conn;
		delete pConn;
	}

	freeList_.clear();
	curConnCnt_ = 0;
}

int CachePool::init()
{
	for (int i = 0; i < curConnCnt_; i++) {
		CacheConn *pConn = new CacheConn(
			serverIp_.c_str(), serverPort_, dbIndex_, password_.c_str(), poolName_.c_str());
		if (pConn->init()) {
			delete pConn;
			return 1;
		}

		freeList_.push_back(pConn);
	}

	log_info("cache pool: %s, list size: %lu\n", poolName_.c_str(), freeList_.size());
	return 0;
}

CacheConn *CachePool::getCacheConn()
{
	std::unique_lock<std::mutex> lock;
	if (freeList_.empty())
	{
		if (curConnCnt_ >= maxConnCnt_) {
			cond_.wait(lock, [this]{
				return !freeList_.empty();
			});
		} else {
			CacheConn *cacheConn = new CacheConn(
				serverIp_.c_str(), serverPort_, dbIndex_, password_.c_str(), poolName_.c_str());
			int ret = cacheConn->init();
			if (ret) {
				log_error("Init CacheConn failed\n");
				delete cacheConn;
				return NULL;
			} else {
				freeList_.push_back(cacheConn);
				curConnCnt_++;
				log_info("new cache connection: %s, conn_cnt: %d\n", poolName_.c_str(), curConnCnt_);
			}
		}
	}

	CacheConn *conn = freeList_.front();
	freeList_.pop_front();
	return conn;
}

void CachePool::relCacheConn(CacheConn *cacheConn)
{
	std::unique_lock<std::mutex> lock;
	list<CacheConn *>::iterator it = freeList_.begin();
	for (; it != freeList_.end(); it++) {
		if (*it == cacheConn) {
			break;
		}
	}

	if (it == freeList_.end()) {
		freeList_.push_back(cacheConn);
	}

	cond_.notify_one();
}
