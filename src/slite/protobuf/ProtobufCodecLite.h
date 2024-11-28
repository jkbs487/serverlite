#pragma once

#include "slite/TCPConnection.h"

#include <google/protobuf/message.h>

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

namespace slite {

// namespace protobuf {

// wire format
//
// Field     Length  Content
//
// size      4-byte  M+N+4
// tag       M-byte  could be "RPC0", etc.
// payload   N-byte
// checksum  4-byte  adler32 of tag+payload
//
// This is an internal class, you should use ProtobufCodecT instead.
class ProtobufCodecLite
{
public:

    enum ErrorCode
    {
      kNoError = 0,
      kInvalidLength,
      kCheckSumError,
      kInvalidNameLen,
      kUnknownMessageType,
      kParseError,
    };

    using RawMessageCallback = std::function<bool (const TCPConnectionPtr&,
                                  const std::string&,
                                  int64_t)>;

    using ProtobufMessageCallback = std::function<void (const TCPConnectionPtr&,
                                  const MessagePtr&,
                                  int64_t)>;

    using ErrorCallback = std::function<void (const TCPConnectionPtr&,
                                  std::string,
                                  int64_t,
                                  ErrorCode)>;

    ProtobufCodecLite(const ::google::protobuf::Message* prototype,
      std::string tagArg,
      const ProtobufMessageCallback& messageCb,
      const RawMessageCallback& rawCb = RawMessageCallback(),
      const ErrorCallback& errCb = defaultErrorCallback)
        : prototype_(prototype),
        tag_(tagArg),
        messageCallback_(messageCb),
        rawMessageCallback_(rawCb),
        errorCallback_(errCb),
        kMinMessageLen(static_cast<int>(tagArg.size()) + kChecksumLen)
    {
    }

    virtual ~ProtobufCodecLite() = default;

    void onMessage(const TCPConnectionPtr& conn,
                    std::string& buf,
                    int64_t receiveTime);

    void send(const TCPConnectionPtr& conn,
              const google::protobuf::Message& message);

    void fillEmptyBuffer(std::string& buf, const google::protobuf::Message& message);
    ErrorCode parse(const char* buf, int len, ::google::protobuf::Message* message);
    static const std::string& errorCodeToString(ErrorCode errorCode);
    static google::protobuf::Message* createMessage(const std::string& type_name);
    static void defaultErrorCallback(const slite::TCPConnectionPtr&,
                                    std::string,
                                    int64_t,
                                    ErrorCode);

private:
    static std::string idToTypeName(uint16_t serviceId, uint16_t commandId);
    static std::pair<uint16_t, uint16_t> typeNameToId(std::string typeName);
    
    const ::google::protobuf::Message* prototype_;
    std::string tag_;
    ProtobufMessageCallback messageCallback_;
    RawMessageCallback rawMessageCallback_;
    ErrorCallback errorCallback_;
    const int kMinMessageLen;

    const static int kHeaderLen = sizeof(int32_t);
    const static int kChecksumLen = sizeof(int32_t);
    const static int kMaxMessageLen = 64*1024*1024; // same as codec_stream.h kDefaultTotalBytesLimit
};

template<typename MSG, const char* TAG, typename CODEC=ProtobufCodecLite>  // TAG must be a variable with external linkage, not a string literal
class ProtobufCodecLiteT
{
  static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value, "CODEC should be derived from ProtobufCodecLite");
 public:
  typedef std::shared_ptr<MSG> ConcreteMessagePtr;
  typedef std::function<void (const TCPConnectionPtr&,
                              const ConcreteMessagePtr&,
                              int64_t)> ProtobufMessageCallback;
  typedef ProtobufCodecLite::RawMessageCallback RawMessageCallback;
  typedef ProtobufCodecLite::ErrorCallback ErrorCallback;

  explicit ProtobufCodecLiteT(const ProtobufMessageCallback& messageCb,
                              const RawMessageCallback& rawCb = RawMessageCallback(),
                              const ErrorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
    : messageCallback_(messageCb),
      codec_(&MSG::default_instance(),
             TAG,
             std::bind(&ProtobufCodecLiteT::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
             rawCb,
             errorCb)
  {
  }

  const std::string& tag() const { return codec_.tag(); }

  void send(const TCPConnectionPtr& conn,
            const MSG& message)
  {
    codec_.send(conn, message);
  }

  void onMessage(const TCPConnectionPtr& conn,
                 std::string buf,
                 int64_t receiveTime)
  {
    codec_.onMessage(conn, buf, receiveTime);
  }

  // internal
  void onRpcMessage(const TCPConnectionPtr& conn,
                    const MessagePtr& message,
                    int64_t receiveTime)
  {
    messageCallback_(conn, std::static_pointer_cast<MSG>(message), receiveTime);
  }

  void fillEmptyBuffer(std::string& buf, const MSG& message)
  {
    codec_.fillEmptyBuffer(buf, message);
  }

 private:
  ProtobufMessageCallback messageCallback_;
  CODEC codec_;
};

// } // namespace protobuf

} // namespace slite