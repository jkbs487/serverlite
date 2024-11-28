#pragma once

#include "slite/protobuf/ProtobufCodecLite.h"

namespace slite
{

class RpcMessage;
// class TcpConnection;

// typedef std::shared_ptr<TCPConnection> TCPConnectionPtr;

using RpcMessagePtr = std::shared_ptr<RpcMessage>;

// namespace protorpc
// {
extern const char rpctag[];// = "RPC0";

// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload
//
using RpcCodec = ProtobufCodecLiteT<RpcMessage, rpctag>;

// } // namespace protobuf
}  // namespace slite
