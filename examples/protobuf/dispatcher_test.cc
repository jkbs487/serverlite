#include "dispatcher.h"

#include "query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

typedef std::shared_ptr<muduo::Query> QueryPtr;
typedef std::shared_ptr<muduo::Answer> AnswerPtr;

void test_down_pointer_cast()
{
  ::std::shared_ptr<google::protobuf::Message> msg(new muduo::Query);
  ::std::shared_ptr<muduo::Query> query(down_pointer_cast<muduo::Query>(msg));
  assert(msg && query);
  if (!query)
  {
    abort();
  }
}

void onQuery(const tcpserver::TCPConnectionPtr&,
             const QueryPtr& message,
             int64_t)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
}

void onAnswer(const tcpserver::TCPConnectionPtr&,
              const AnswerPtr& message,
              int64_t)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const tcpserver::TCPConnectionPtr&,
                          const MessagePtr& message,
                          int64_t)
{
  cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  test_down_pointer_cast();

  ProtobufDispatcher dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback<muduo::Query>(onQuery);
  dispatcher.registerMessageCallback<muduo::Answer>(onAnswer);

  tcpserver::TCPConnectionPtr conn;
  int64_t t = 0;

  std::shared_ptr<muduo::Query> query(new muduo::Query);
  std::shared_ptr<muduo::Answer> answer(new muduo::Answer);
  std::shared_ptr<muduo::Empty> empty(new muduo::Empty);
  dispatcher.onProtobufMessage(conn, query, t);
  dispatcher.onProtobufMessage(conn, answer, t);
  dispatcher.onProtobufMessage(conn, empty, t);

  google::protobuf::ShutdownProtobufLibrary();
}

