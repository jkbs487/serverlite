//
//  UserInfo.cpp
//  im-server-TT
//
//  Created by luoning on 14-10-23.
//  Copyright (c) 2014年 luoning. All rights reserved.
//

#include "UserManager.h"
#include "base/public_define.h"
#include "pbs/IM.BaseDefine.pb.h"

using namespace IM;
using namespace slite;
using namespace IM::BaseDefine;

UserManager::UserManager()
{
}

UserManager::~UserManager()
{
}

void UserManager::addClientType(uint32_t clientType)
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.find(clientType);
    if (it != clientTypeList_.end()) {
        it->second += 1;
    } else {
        clientTypeList_[clientType] = 1;
    }
}

void UserManager::removeClientType(uint32_t clientType)
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.find(clientType);
    if (it != clientTypeList_.end()) {
        uint32_t count = it->second;
        count -= 1;
        if (count > 0) {
            it->second = count;
        } else {
            clientTypeList_.erase(clientType);
        }
    }
}

bool UserManager::findRouteConn(TCPConnectionPtr conn)
{
    set<TCPConnectionPtr>::iterator it = routeConnSet_.find(conn);
    if (it != routeConnSet_.end()) {
        return true;
    } else {
        return false;
    }
}

uint32_t UserManager::getCountByClientType(uint32_t clientType)
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.find(clientType);
    if (it != clientTypeList_.end()) {
        return it->second;
    } else {
        return 0;
    }
}

bool UserManager::isMsgConnNULL()
{
    if (clientTypeList_.empty()) {
        return true;
    } else {
        return false;
    }
}

void UserManager::clearClientType()
{
    clientTypeList_.clear();
}

bool UserManager::isPCClientLogin()
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.begin();
    for (; it != clientTypeList_.end(); it++) {
        uint32_t clientType = it->first;
        if (CHECK_CLIENT_TYPE_PC(clientType)) {
            return true;
        }
    }
    return false;
}

bool UserManager::isMobileClientLogin()
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.begin();
    for (; it != clientTypeList_.end(); it++) {
        uint32_t clientType = it->first;
        if (CHECK_CLIENT_TYPE_MOBILE(clientType)) {
            return true;
        }
    }
    return false;
}

uint32_t UserManager::getStatus()
{
    map<uint32_t, uint32_t>::iterator it = clientTypeList_.begin();
    for (; it != clientTypeList_.end(); it++) {
        uint32_t clientType = it->first;
        if (CHECK_CLIENT_TYPE_PC(clientType)) {
            return USER_STATUS_ONLINE;
        }
    }
    return USER_STATUS_OFFLINE;
}