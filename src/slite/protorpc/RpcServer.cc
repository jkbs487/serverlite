// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "slite/protorpc/RpcServer.h"

#include "slite/Logger.h"
#include "slite/protorpc/RpcChannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace slite;
using namespace std::placeholders;

RpcServer::RpcServer(EventLoop* loop,
                     const std::string& host, 
                     uint32_t port)
  : server_(host, port, loop, "RpcServer")
{
  server_.setConnectionCallback(
      std::bind(&RpcServer::onConnection, this, _1));
//   server_.setMessageCallback(
//       std::bind(&RpcServer::onMessage, this, _1, _2, _3));
}

void RpcServer::registerService(google::protobuf::Service* service)
{
  const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
  services_[desc->full_name()] = service;
}

void RpcServer::start()
{
  server_.start();
}

void RpcServer::onConnection(const TCPConnectionPtr& conn)
{
  LOG_INFO << "RpcServer - " << conn->peerPort() << " -> "
    << conn->localPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
  if (conn->connected())
  {
    RpcChannelPtr channel(new RpcChannel(conn));
    channel->setServices(&services_);
    conn->setMessageCallback(
        std::bind(&RpcChannel::onMessage, channel.get(), _1, _2, _3));
    conn->setContext(channel);
  }
  else
  {
    conn->setContext(RpcChannelPtr());
    // FIXME:
  }
}

// void RpcServer::onMessage(const TcpConnectionPtr& conn,
//                           Buffer* buf,
//                           Timestamp time)
// {
//   RpcChannelPtr& channel = boost::any_cast<RpcChannelPtr&>(conn->getContext());
//   channel->onMessage(conn, buf, time);
// }

