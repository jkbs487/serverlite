#include "slite/protobuf/ProtobufCodecLite.h"

#include "slite/Logger.h"

#include "google-inl.h"

#include <google/protobuf/descriptor.h>

#include <zlib.h>  // adler32
#include <string>
#include <arpa/inet.h>

using namespace slite;
// using namespace slite::protobuf;

namespace
{
  int ProtobufVersionCheck()
  {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
  }
  int __attribute__ ((unused)) dummy = ProtobufVersionCheck();
}

void ProtobufCodecLite::send(const TCPConnectionPtr& conn,
                             const ::google::protobuf::Message& message)
{
  // FIXME: serialize to TcpConnection::outputBuffer()
  std::string buf;
  fillEmptyBuffer(buf, message);
  conn->send(buf);
}

void ProtobufCodecLite::fillEmptyBuffer(std::string& buf, const google::protobuf::Message& message)
{
  assert(buf.size() == 0);
  buf.append(tag_);
  // code copied from MessageLite::SerializeToArray() and MessageLite::SerializePartialToArray().
  GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

  /**
   * 'ByteSize()' of message is deprecated in Protocol Buffers v3.4.0 firstly.
   * But, till to v3.11.0, it just getting start to be marked by '__attribute__((deprecated()))'.
   * So, here, v3.9.2 is selected as maximum version using 'ByteSize()' to avoid
   * potential effect for previous muduo code/projects as far as possible.
   * Note: All information above just INFER from
   * 1) https://github.com/protocolbuffers/protobuf/releases/tag/v3.4.0
   * 2) MACRO in file 'include/google/protobuf/port_def.inc'.
   * eg. '#define PROTOBUF_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))'.
   * In addition, usage of 'ToIntSize()' comes from Impl of ByteSize() in new version's Protocol Buffers.
   */

  #if GOOGLE_PROTOBUF_VERSION > 3009002
    int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
  #else
    int byte_size = message.ByteSize();
  #endif
  
  buf.resize(buf.size() + byte_size);

  uint8_t* start = reinterpret_cast<uint8_t*>(buf.data() + tag_.size());
  uint8_t* end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size)
  {
    #if GOOGLE_PROTOBUF_VERSION > 3009002
      ByteSizeConsistencyError(byte_size, google::protobuf::internal::ToIntSize(message.ByteSizeLong()), static_cast<int>(end - start));
    #else
      ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
    #endif
  }
  
  int32_t checkSum = htonl(static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(buf.data()),
                static_cast<int>(buf.size()))));
  buf.append(reinterpret_cast<const char*>(&checkSum), kChecksumLen);
  assert(buf.size() == tag_.size() + byte_size + kChecksumLen);
  int32_t len = htonl(static_cast<int32_t>(buf.size()));
  buf.insert(0, reinterpret_cast<const char*>(&len), sizeof len);
}

//
// no more google code after this
//

//
// FIXME: merge with RpcCodec
//

namespace
{
  const std::string kNoErrorStr = "NoError";
  const std::string kInvalidLengthStr = "InvalidLength";
  const std::string kCheckSumErrorStr = "CheckSumError";
  const std::string kInvalidNameLenStr = "InvalidNameLen";
  const std::string kUnknownMessageTypeStr = "UnknownMessageType";
  const std::string kParseErrorStr = "ParseError";
  const std::string kUnknownErrorStr = "UnknownError";
}

const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode)
{
  switch (errorCode)
  {
   case kNoError:
     return kNoErrorStr;
   case kInvalidLength:
     return kInvalidLengthStr;
   case kCheckSumError:
     return kCheckSumErrorStr;
   case kInvalidNameLen:
     return kInvalidNameLenStr;
   case kUnknownMessageType:
     return kUnknownMessageTypeStr;
   case kParseError:
     return kParseErrorStr;
   default:
     return kUnknownErrorStr;
  }
}

void ProtobufCodecLite::defaultErrorCallback(const slite::TCPConnectionPtr& conn,
                                         std::string buf,
                                         int64_t,
                                         ErrorCode errorCode)
{
  LOG_ERROR << "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
  if (conn && conn->connected())
  {
    conn->shutdown();
  }
}

int32_t asInt32(const char* buf)
{
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return ntohl(be32);
}

void ProtobufCodecLite::onMessage(const slite::TCPConnectionPtr& conn,
                              std::string& buf,
                              int64_t receiveTime)
{
  while (buf.size() >= static_cast<size_t>(kMinMessageLen + kHeaderLen)) {
    int32_t len;
    ::memcpy(&len, buf.data(), sizeof len);
    len = ntohl(len);
    if (len > kMaxMessageLen || len < kMinMessageLen) {
      errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    }
    else if (buf.size() >= static_cast<size_t>(len + kHeaderLen)) {
      if (rawMessageCallback_ && !rawMessageCallback_(conn, std::string(buf.data() + kHeaderLen, len), receiveTime)) {
        buf = buf.substr(kHeaderLen + len, buf.size()-kHeaderLen-len);
        continue;
      }
      MessagePtr message(prototype_->New());
      ErrorCode errorCode = parse(buf.data()+kHeaderLen, len, message.get());
      if (errorCode == kNoError) {
        messageCallback_(conn, message, receiveTime);
        buf = buf.substr(kHeaderLen + len, buf.size()-kHeaderLen-len);
      } else {
        errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    } else {
      break;
    }
  }
}

google::protobuf::Message* ProtobufCodecLite::createMessage(const std::string& typeName)
{
  google::protobuf::Message* message = NULL;
  const google::protobuf::Descriptor* descriptor =
    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
  if (descriptor)
  {
    const google::protobuf::Message* prototype =
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (prototype)
    {
      message = prototype->New();
    }
  }
  return message;
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(const char* buf, int len, ::google::protobuf::Message* message)
{
  ErrorCode error = kNoError;
  // check sum
  int32_t expectedCheckSum = asInt32(buf + len - kHeaderLen);
  int32_t checkSum = static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(buf),
                static_cast<int>(len - kHeaderLen)));
  if (checkSum == expectedCheckSum) {
    if (memcmp(buf + kHeaderLen, tag_.data(), tag_.size())) {
      if (message) {
        // parse from buffer
        const char* data = buf + tag_.size();
        int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
        if (message->ParseFromArray(data, dataLen)) {
          error = kNoError;
        } else {
          error = kParseError;
        }
      } else {
        error = kUnknownMessageType;
      }
    } else {
      error = kInvalidNameLen;
    }
  } else {
    error = kCheckSumError;
  }

  return error;
}