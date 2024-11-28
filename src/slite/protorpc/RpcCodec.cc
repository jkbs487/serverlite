#include "slite/protorpc/RpcCodec.h"
#include "slite/TCPConnection.h"

#include "slite/protorpc/rpc.pb.h"
#include "slite/protorpc/google-inl.h"

using namespace slite;
// using namespace slite::protorpc;

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
// namespace protorpc
// {
const char rpctag [] = "RPC0";
// }
}