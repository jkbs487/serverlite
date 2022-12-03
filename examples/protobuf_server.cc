#include "slite/protobuf/ProtobufCodec.h"
#include "slite/protobuf/ProtobufDispatcher.h"
#include "slite/test/query.pb.h"

#include "slite/Logger.h"
#include "slite/EventLoop.h"
#include "slite/TCPServer.h"

#include <stdio.h>
#include <unistd.h>
#include <functional>

using namespace slite;
using namespace slite::protobuf;
using namespace std::placeholders;

typedef std::shared_ptr<slite::Query> QueryPtr;
typedef std::shared_ptr<slite::Answer> AnswerPtr;

class QueryServer
{
 public:
  QueryServer(EventLoop* loop,
              const std::string& addr,
              uint16_t port)
  : server_(addr, port, loop, "QueryServer"),
    dispatcher_(std::bind(&QueryServer::onUnknownMessage, this, _1, _2, _3)),
    codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, _1, _2, _3))
  {
    dispatcher_.registerMessageCallback<slite::Query>(
        std::bind(&QueryServer::onQuery, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<slite::Answer>(
        std::bind(&QueryServer::onAnswer, this, _1, _2, _3));
    server_.setConnectionCallback(
        std::bind(&QueryServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
  }

  void start()
  {
    server_.start();
  }

 private:
  void onConnection(const TCPConnectionPtr& conn)
  {
    LOG_INFO << conn->peerAddr() << ":" << conn->peerPort() << " -> "
        << conn->localAddr() << ":" << conn->localPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
  }

  void onUnknownMessage(const TCPConnectionPtr& conn,
                        const MessagePtr& message,
                        uint64_t)
  {
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
  }

  void onQuery(const TCPConnectionPtr& conn,
               const QueryPtr& message,
               uint64_t)
  {
    LOG_INFO << "onQuery:\n" << message->GetTypeName() << message->DebugString();
    Answer answer;
    answer.set_id(1);
    answer.set_questioner("Chen Shuo");
    answer.set_answerer("blog.csdn.net/Solstice");
    answer.add_solution("Jump!");
    answer.add_solution("Win!");
    codec_.send(conn, answer);

    conn->shutdown();
  }

  void onAnswer(const TCPConnectionPtr& conn,
                const AnswerPtr& message,
                uint64_t)
  {
    LOG_INFO << "onAnswer: " << message->GetTypeName();
    conn->shutdown();
  }

  TCPServer server_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    QueryServer server(&loop, "0.0.0.0", port);
    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}

