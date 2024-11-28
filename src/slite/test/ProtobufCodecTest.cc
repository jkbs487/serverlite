#include "slite/protobuf/ProtobufCodec.h"
#include "slite/test/query.pb.h"

#include <stdio.h>
#include <zlib.h>  // adler32
#include <arpa/inet.h>

using namespace slite;
// using namespace slite::protobuf;

template<typename To, typename From>
inline To implicit_cast(From const &f)
{
  return f;
}

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in muduo/base/Types.h
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  if (false)
  {
    implicit_cast<From*, To*>(0);
  }

#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

void print(const std::string buf)
{
  printf("encoded to %zd bytes\n", buf.size());
  for (size_t i = 0; i < buf.size(); ++i)
  {
    unsigned char ch = static_cast<unsigned char>(buf.data()[i]);

    printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

void testQuery()
{
  slite::Query query;
  query.set_id(1);
  query.set_questioner("Chen Shuo");
  query.add_question("Running?");

  std::string buf;
  ProtobufCodec::fillEmptyBuffer(buf, query);
  print(buf);

  int32_t len;
  ::memcpy(&len, buf.data(), sizeof len);
  len = ntohl(len);
  assert(len+4 == static_cast<int32_t>(buf.size()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buf.data()+4, len, &errorCode);
  assert(errorCode == ProtobufCodec::kNoError);
  assert(message != NULL);
  message->PrintDebugString();
  assert(message->DebugString() == query.DebugString());

  std::shared_ptr<slite::Query> newQuery = down_pointer_cast<slite::Query>(message);
  assert(newQuery != NULL);
}

void testAnswer()
{
  slite::Answer answer;
  answer.set_id(1);
  answer.set_questioner("Chen Shuo");
  answer.set_answerer("blog.csdn.net/Solstice");
  answer.add_solution("Jump!");
  answer.add_solution("Win!");

  std::string buf;
  ProtobufCodec::fillEmptyBuffer(buf, answer);
  print(buf);

  int32_t len;
  ::memcpy(&len, buf.data(), sizeof len);
  len = ntohl(len);
  assert(len+4 == static_cast<int32_t>(buf.size()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buf.data()+4, len, &errorCode);
  assert(errorCode == ProtobufCodec::kNoError);
  assert(message != NULL);
  message->PrintDebugString();
  assert(message->DebugString() == answer.DebugString());

  std::shared_ptr<slite::Answer> newAnswer = down_pointer_cast<slite::Answer>(message);
  assert(newAnswer != NULL);
}

void testEmpty()
{
  slite::Empty empty;

  std::string buf;
  ProtobufCodec::fillEmptyBuffer(buf, empty);
  print(buf);

  int32_t len;
  ::memcpy(&len, buf.data(), sizeof len);
  len = ntohl(len);
  assert(len+4 == static_cast<int32_t>(buf.size()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buf.data()+4, len, &errorCode);
  assert(message != NULL);
  message->PrintDebugString();
  assert(message->DebugString() == empty.DebugString());
}

void redoCheckSum(std::string& data, int len)
{
  int32_t checkSum = static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(data.c_str()),
                static_cast<int>(len - 4)));
  data[len-4] = reinterpret_cast<const char*>(&checkSum)[0];
  data[len-3] = reinterpret_cast<const char*>(&checkSum)[1];
  data[len-2] = reinterpret_cast<const char*>(&checkSum)[2];
  data[len-1] = reinterpret_cast<const char*>(&checkSum)[3];
}

void testBadBuffer()
{
  slite::Empty empty;
  empty.set_id(43);

  std::string buf;
  ProtobufCodec::fillEmptyBuffer(buf, empty);
  // print(buf);

  int32_t len;
  ::memcpy(&len, buf.data(), sizeof len);
  len = ntohl(len);
  assert(len+4 == static_cast<int32_t>(buf.size()));
  buf = buf.substr(4, buf.size()-4);
  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len-1, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }

  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[len-1]++;
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }

  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[0]++;
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }
/*
  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3] = 0;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kInvalidNameLen);
  }

  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3] = 100;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kInvalidNameLen);
  }

  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3]--;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kUnknownMessageType);
  }

  {
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[4] = 'M';
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kUnknownMessageType);
  }

  {
    // FIXME: reproduce parse error
    std::string data(buf.data(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    // assert(message == NULL);
    // assert(errorCode == ProtobufCodec::kParseError);
  }
  */
}

int g_count = 0;

void onMessage(const slite::TCPConnectionPtr& conn,
               const MessagePtr& message,
               int64_t receiveTime)
{
  printf("onMessage: %s\n", message->GetTypeName().c_str());
  g_count++;
}

void testOnMessage()
{
  slite::Query query;
  query.set_id(1);
  query.set_questioner("Chen Shuo");
  query.add_question("Running?");

  std::string buf1;
  ProtobufCodec::fillEmptyBuffer(buf1, query);

  slite::Empty empty;
  empty.set_id(43);
  empty.set_id(1982);

  std::string buf2;
  ProtobufCodec::fillEmptyBuffer(buf2, empty);

  size_t totalLen = buf1.size() + buf2.size();
  std::string all;
  all.append(buf1.data(), buf1.size());
  all.append(buf2.data(), buf2.size());
  assert(all.size() == totalLen);
  slite::TCPConnectionPtr conn;
  int64_t t = 0;
  ProtobufCodec codec(onMessage);
  for (size_t len = 0; len <= totalLen; ++len)
  {
    std::string input;
    input.append(all.data(), len);

    g_count = 0;
    codec.onMessage(conn, input, t);
    int expected = len < buf1.size() ? 0 : 1;
    if (len == totalLen) expected = 2;
    assert(g_count == expected); (void) expected;
    //printf("%2zd %d\n", len, g_count);

    input.append(all.data() + len, totalLen - len);
    codec.onMessage(conn, input, t);
    assert(g_count == 2);
  }
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  testQuery();
  puts("");
  testAnswer();
  puts("");
  testEmpty();
  puts("");
  testBadBuffer();
  puts("");
  testOnMessage();
  puts("");

  puts("All pass!!!");

  google::protobuf::ShutdownProtobufLibrary();
}

