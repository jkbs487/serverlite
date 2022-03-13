//
//  UserInfo.h
//  im-server-TT
//
//  Created by luoning on 14-10-23.
//  Copyright (c) 2014年 luoning. All rights reserved.
//
#pragma once

#include <iostream>
#include <set>
#include <map>

#include "slite/TCPConnection.h"

using namespace std;

namespace IM {

class UserManager
{
public:
    UserManager();
    ~UserManager();
    
    uint32_t getStatus();
     
    void addRouteConn(slite::TCPConnectionPtr conn) { routeConnSet_.insert(conn); }
    void removeRouteConn(slite::TCPConnectionPtr conn) { routeConnSet_.erase(conn); }
    
    void clearRouteConn() { routeConnSet_.clear(); }
    set<slite::TCPConnectionPtr>* getRouteConn() { return &routeConnSet_; }
    bool findRouteConn(slite::TCPConnectionPtr pConn);
    
    size_t getRouteConnCount() { return routeConnSet_.size(); }
    void addClientType(uint32_t clientType);
    void removeClientType(uint32_t clientType);
    uint32_t getCountByClientType(uint32_t clientType);
    bool isMsgConnNULL();
    void clearClientType();
    
    bool isPCClientLogin();
    bool isMobileClientLogin();
private:
    set<slite::TCPConnectionPtr> routeConnSet_;

    // first: clientType, second: count
    map<uint32_t, uint32_t> clientTypeList_;
};

}