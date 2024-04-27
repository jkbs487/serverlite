#include "slite/protobuf/ProtobufCodec.h"
#include "slite/protobuf/ProtobufDispatcher.h"
#include "slite/test/query.pb.h"

#include "slite/Logger.h"
#include "slite/EventLoop.h"
#include "slite/TCPClient.h"

#include <stdio.h>
#include <unistd.h>
#include <functional>

using namespace slite;
// using namespace slite::protobuf;
using namespace std::placeholders;

typedef std::shared_ptr<slite::Empty> EmptyPtr;
typedef std::shared_ptr<slite::Answer> AnswerPtr;

google::protobuf::Message* messageToSend;

class QueryClient
{
 public:
  QueryClient(EventLoop* loop,
              const std::string& addr,
              uint16_t port)
  : loop_(loop),
    client_(addr, port, loop, "QueryClient"),
    dispatcher_(std::bind(&QueryClient::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
  {
    dispatcher_.registerMessageCallback<slite::Answer>(
        std::bind(&QueryClient::onAnswer, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<slite::Empty>(
        std::bind(&QueryClient::onEmpty, this, _1, _2, _3));
    client_.setConnectionCallback(
        std::bind(&QueryClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
  }

  void connect()
  {
    client_.connect();
  }

 private:

  void onConnection(const TCPConnectionPtr& conn)
  {
    LOG_INFO << conn->peerAddr() << ":" << conn->peerPort() << " -> "
        << conn->localAddr() << ":" << conn->localPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      codec_.send(conn, *messageToSend);
    } else {
      loop_->quit();
    }
  }

  void onUnknownMessage(const TCPConnectionPtr&,
                        const MessagePtr& message,
                        uint64_t)
  {
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
  }

  void onAnswer(const TCPConnectionPtr&,
                const AnswerPtr& message,
                uint64_t)
  {
    LOG_INFO << "onAnswer:\n" << message->GetTypeName() << message->DebugString();
  }

  void onEmpty(const TCPConnectionPtr&,
               const EmptyPtr& message,
               uint64_t)
  {
    LOG_INFO << "onEmpty: " << message->GetTypeName();
  }

  EventLoop* loop_;
  TCPClient client_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 2)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));

    slite::Query query;
    query.set_id(1);
    query.set_questioner("Chen Shuo");
    query.add_question("Running?");
    slite::Empty empty;
    messageToSend = &query;

    if (argc > 3 && argv[3][0] == 'e')
    {
      messageToSend = &empty;
    }

    QueryClient client(&loop, argv[1], port);
    client.connect();
    loop.loop();
  }
  else
  {
    printf("Usage: %s host_ip port [q|e]\n", argv[0]);
  }
}

