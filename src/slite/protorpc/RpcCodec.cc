// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "slite/protorpc/RpcCodec.h"
#include "slite/TCPConnection.h"

#include "slite/protorpc/rpc.pb.h"
#include "slite/protorpc/google-inl.h"

using namespace slite;

namespace
{
  int ProtobufVersionCheck()
  {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
  }
  int dummy __attribute__ ((unused)) = ProtobufVersionCheck();
}

namespace slite
{

const char rpctag [] = "RPC0";
}