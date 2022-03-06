// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_EXAMPLES_PROTOBUF_CODEC_CODEC_H
#define MUDUO_EXAMPLES_PROTOBUF_CODEC_CODEC_H

#include "slite/TCPConnection.h"

#include <google/protobuf/message.h>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

//
// FIXME: merge with RpcCodec
//
class ProtobufCodec
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

    typedef std::function<void (const slite::TCPConnectionPtr&,
                                  const MessagePtr&,
                                  int64_t)> ProtobufMessageCallback;

    typedef std::function<void (const slite::TCPConnectionPtr&,
                                  std::string,
                                  int64_t,
                                  ErrorCode)> ErrorCallback;

    explicit ProtobufCodec(const ProtobufMessageCallback& messageCb)
        : messageCallback_(messageCb),
        errorCallback_(defaultErrorCallback)
    {
    }

    ProtobufCodec(const ProtobufMessageCallback& messageCb, const ErrorCallback& errorCb)
        : messageCallback_(messageCb),
        errorCallback_(errorCb)
    {
    }

    void onMessage(const slite::TCPConnectionPtr& conn,
                    std::string& buf,
                    int64_t receiveTime);

    void send(const slite::TCPConnectionPtr& conn,
              const google::protobuf::Message& message)
    {
        // FIXME: serialize to TcpConnection::outputBuffer()
        std::string buf;
        fillEmptyBuffer(buf, message);
        conn->send(buf);
    }

    static const std::string& errorCodeToString(ErrorCode errorCode);
    static void fillEmptyBuffer(std::string& buf, const google::protobuf::Message& message);
    static google::protobuf::Message* createMessage(const std::string& type_name);
    static MessagePtr parse(const char* buf, int len, ErrorCode* errorCode);

private:
    static void defaultErrorCallback(const slite::TCPConnectionPtr&,
                                    std::string,
                                    int64_t,
                                    ErrorCode);

    static std::string idToTypeName(uint16_t serviceId, uint16_t commandId);
    static std::pair<uint16_t, uint16_t> typeNameToId(std::string typeName);
    ProtobufMessageCallback messageCallback_;
    ErrorCallback errorCallback_;

    const static int kHeaderLen = sizeof(int32_t);
    const static int kMinMessageLen = 2*kHeaderLen + 2; // nameLen + typeName + checkSum
    const static int kMaxMessageLen = 64*1024*1024; // same as codec_stream.h kDefaultTotalBytesLimit
};

#endif  // MUDUO_EXAMPLES_PROTOBUF_CODEC_CODEC_H
