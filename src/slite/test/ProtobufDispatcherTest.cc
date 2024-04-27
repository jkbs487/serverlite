#include "slite/protobuf/ProtobufDispatcher.h"
#include "slite/test/query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

typedef std::shared_ptr<slite::Query> QueryPtr;
typedef std::shared_ptr<slite::Answer> AnswerPtr;

void test_down_pointer_cast()
{
  ::std::shared_ptr<google::protobuf::Message> msg(new slite::Query);
  ::std::shared_ptr<slite::Query> query(down_pointer_cast<slite::Query>(msg));
  assert(msg && query);
  if (!query)
  {
    abort();
  }
}

void onQuery(const slite::TCPConnectionPtr&,
             const QueryPtr& message,
             int64_t)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
}

void onAnswer(const slite::TCPConnectionPtr&,
              const AnswerPtr& message,
              int64_t)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const slite::TCPConnectionPtr&,
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
  dispatcher.registerMessageCallback<slite::Query>(onQuery);
  dispatcher.registerMessageCallback<slite::Answer>(onAnswer);

  slite::TCPConnectionPtr conn;
  int64_t t = 0;

  std::shared_ptr<slite::Query> query(new slite::Query);
  std::shared_ptr<slite::Answer> answer(new slite::Answer);
  std::shared_ptr<slite::Empty> empty(new slite::Empty);
  dispatcher.onProtobufMessage(conn, query, t);
  dispatcher.onProtobufMessage(conn, answer, t);
  dispatcher.onProtobufMessage(conn, empty, t);

  google::protobuf::ShutdownProtobufLibrary();
}

