// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#pragma once

#include "slite/TCPServer.h"

namespace google {
namespace protobuf {

class Service;

}  // namespace protobuf
}  // namespace google

namespace slite
{

class RpcServer
{
 public:
  RpcServer(EventLoop* loop,
            const std::string& host, uint32_t port);

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void registerService(::google::protobuf::Service*);
  void start();

 private:
  void onConnection(const TCPConnectionPtr& conn);

  // void onMessage(const TcpConnectionPtr& conn,
  //                Buffer* buf,
  //                Timestamp time);

  TCPServer server_;
  std::map<std::string, ::google::protobuf::Service*> services_;
};

}  // namespace slite