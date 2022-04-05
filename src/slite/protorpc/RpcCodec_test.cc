#undef NDEBUG
#include "slite/protorpc/RpcCodec.h"
#include "slite/protorpc/rpc.pb.h"
#include "slite/protobuf/codec.h"

#include <stdio.h>

using namespace slite;

void rpcMessageCallback(const TCPConnectionPtr&,
                        const RpcMessagePtr&,
                        int64_t)
{
}

MessagePtr g_msgptr;
void messageCallback(const TCPConnectionPtr&,
                     const MessagePtr& msg,
                     int64_t)
{
  g_msgptr = msg;
}

void print(const std::string& buf)
{
  printf("encoded to %zd bytes\n", buf.size());
  for (size_t i = 0; i < buf.size(); ++i)
  {
    unsigned char ch = static_cast<unsigned char>(buf[i]);

    printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

char rpctag[] = "RPC0";

int main()
{
  RpcMessage message;
  message.set_type(REQUEST);
  message.set_id(2);
  char wire[] = "\0\0\0\x13" "RPC0" "\x08\x01\x11\x02\0\0\0\0\0\0\0" "\x0f\xef\x01\x32";
  std::string expected(wire, sizeof(wire)-1);
  std::string s1, s2;
  std::string buf1, buf2;
  {
  RpcCodec codec(rpcMessageCallback);
  codec.fillEmptyBuffer(&buf1, message);
  print(buf1);
  s1 = buf1;
  }

  {
  ProtobufCodec codec(&RpcMessage::default_instance(), messageCallback);
  codec.fillEmptyBuffer(buf2, message);
  print(buf2);
  s2 = buf2;
  codec.onMessage(TcpConnectionPtr(), &buf1, 0);
  assert(g_msgptr);
  assert(g_msgptr->DebugString() == message.DebugString());
  g_msgptr.reset();
  }
  assert(s1 == s2);
  assert(s1 == expected);
  assert(s2 == expected);

  {
  std::string buf;
  ProtobufCodec codec(&RpcMessage::default_instance(), "XYZ", messageCallback);
  codec.fillEmptyBuffer(&buf, message);
  print(buf);
  s2 = buf;
  codec.onMessage(TcpConnectionPtr(), &buf, Timestamp::now());
  assert(g_msgptr);
  assert(g_msgptr->DebugString() == message.DebugString());
  }

  google::protobuf::ShutdownProtobufLibrary();
}
