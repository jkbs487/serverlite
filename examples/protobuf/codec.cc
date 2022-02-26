// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "codec.h"

#include "slite/Logger.h"

#include "google-inl.h"

#include <google/protobuf/descriptor.h>

#include <zlib.h>  // adler32
#include <string>
#include <arpa/inet.h>

using namespace slite;

uint16_t g_seqNum;

void ProtobufCodec::fillEmptyBuffer(std::string& buf, const google::protobuf::Message& message)
{
  // buf->retrieveAll();
  assert(buf.size() == 0);
  const std::string& typeName = message.GetTypeName();
  int32_t nameLen = static_cast<int32_t>(typeName.size()+1);
  int32_t nNameLen = htonl(nameLen);
  buf.append(reinterpret_cast<const char*>(&nNameLen), sizeof nameLen);
  buf.append(typeName.c_str(), nameLen);
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

  uint8_t* start = reinterpret_cast<uint8_t*>(buf.data() + nameLen + sizeof nameLen);
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
  buf.append(reinterpret_cast<const char*>(&checkSum), sizeof checkSum);
  assert(buf.size() == sizeof nameLen + nameLen + byte_size + sizeof checkSum);
  int32_t len = htonl(static_cast<int32_t>(buf.size()));
  buf.insert(0, reinterpret_cast<const char*>(&len), sizeof len);
}

void ProtobufCodec::fillEmptyBuffer2(std::string& buf, const google::protobuf::Message& message)
{
  // buf->retrieveAll();
  assert(buf.size() == 0);
  std::pair<uint16_t, uint16_t> id = typeNameToId(message.GetTypeName());
  uint16_t version = htons(1);
  uint16_t appid = htons(0);
  uint16_t serviceId = htons(id.first);
  uint16_t commandId = htons(id.second);
  uint16_t seqNum = htons(g_seqNum);
  uint16_t reserve = htons(0);
  buf.append(reinterpret_cast<const char*>(&version), sizeof version);
  buf.append(reinterpret_cast<const char*>(&appid), sizeof appid);
  buf.append(reinterpret_cast<const char*>(&serviceId), sizeof serviceId);
  buf.append(reinterpret_cast<const char*>(&commandId), sizeof commandId);
  buf.append(reinterpret_cast<const char*>(&seqNum), sizeof seqNum);
  buf.append(reinterpret_cast<const char*>(&reserve), sizeof reserve);
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

  uint8_t* start = reinterpret_cast<uint8_t*>(buf.data() + kHeaderLen2 - 4);
  uint8_t* end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size)
  {
    #if GOOGLE_PROTOBUF_VERSION > 3009002
      ByteSizeConsistencyError(byte_size, google::protobuf::internal::ToIntSize(message.ByteSizeLong()), static_cast<int>(end - start));
    #else
      ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
    #endif
  }
  
  int32_t len = htonl(static_cast<int32_t>(buf.size()+4));
  std::string lenStr;
  lenStr.append(reinterpret_cast<const char*>(&len), sizeof len);
  buf.insert(0, lenStr);
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

const std::string& ProtobufCodec::errorCodeToString(ErrorCode errorCode)
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

void ProtobufCodec::defaultErrorCallback(const slite::TCPConnectionPtr& conn,
                                         std::string buf,
                                         int64_t,
                                         ErrorCode errorCode)
{
  LOG_ERROR << "ProtobufCodec::defaultErrorCallback - " << errorCodeToString(errorCode);
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

uint16_t asUint16(const char* buf)
{
  uint16_t be16 = 0;
  ::memcpy(&be16, buf, sizeof(be16));
  return ntohs(be16);
}

void ProtobufCodec::onMessage(const slite::TCPConnectionPtr& conn,
                              std::string& buf,
                              int64_t receiveTime)
{
  while (buf.size() >= kMinMessageLen + kHeaderLen)
  {
    int32_t len;
    ::memcpy(&len, buf.data(), sizeof len);
    len = ntohl(len);
    if (len > kMaxMessageLen || len < kMinMessageLen)
    {
      errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    }
    else if (buf.size() >= static_cast<size_t>(len + kHeaderLen))
    {
      ErrorCode errorCode = kNoError;
      MessagePtr message = parse(buf.data()+kHeaderLen, len, &errorCode);
      if (errorCode == kNoError && message)
      {
        messageCallback_(conn, message, receiveTime);
        buf = buf.substr(kHeaderLen + len, buf.size()-kHeaderLen-len);
      }
      else
      {
        errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    }
    else
    {
      break;
    }
  }
}

void ProtobufCodec::onMessage2(const slite::TCPConnectionPtr& conn,
                              std::string& buf,
                              int64_t receiveTime)
{
  while (buf.size() >= kMinMessageLen2 + kHeaderLen2)
  {
    int32_t len;
    ::memcpy(&len, buf.data(), sizeof len);
    len = ntohl(len);
    if (len > kMaxMessageLen2 || len < kMinMessageLen2)
    {
      errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    }
    else if (buf.size() >= static_cast<size_t>(len))
    {
      ErrorCode errorCode = kNoError;
      MessagePtr message = parse2(buf.data()+kHeaderLen, len-kHeaderLen, &errorCode);
      if (errorCode == kNoError && message)
      {
        messageCallback_(conn, message, receiveTime);
        buf = buf.substr(len, buf.size()-len);
      }
      else
      {
        errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    }
    else
    {
      break;
    }
  }
}

google::protobuf::Message* ProtobufCodec::createMessage(const std::string& typeName)
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

MessagePtr ProtobufCodec::parse(const char* buf, int len, ErrorCode* error)
{
  MessagePtr message;

  // check sum
  int32_t expectedCheckSum = asInt32(buf + len - kHeaderLen);
  int32_t checkSum = static_cast<int32_t>(
      ::adler32(1,
                reinterpret_cast<const Bytef*>(buf),
                static_cast<int>(len - kHeaderLen)));
  if (checkSum == expectedCheckSum)
  {
    // get message type name
    int32_t nameLen = asInt32(buf);
    if (nameLen >= 2 && nameLen <= len - 2*kHeaderLen)
    {
      std::string typeName(buf + kHeaderLen, buf + kHeaderLen + nameLen - 1);
      // create message object
      message.reset(createMessage(typeName));
      if (message)
      {
        // parse from buffer
        const char* data = buf + kHeaderLen + nameLen;
        int32_t dataLen = len - nameLen - 2*kHeaderLen;
        if (message->ParseFromArray(data, dataLen))
        {
          *error = kNoError;
        }
        else
        {
          *error = kParseError;
        }
      }
      else
      {
        *error = kUnknownMessageType;
      }
    }
    else
    {
      *error = kInvalidNameLen;
    }
  }
  else
  {
    *error = kCheckSumError;
  }

  return message;
}

MessagePtr ProtobufCodec::parse2(const char* buf, int len, ErrorCode* error)
{
    MessagePtr message;

    // check sum
    //uint16_t version = asUint16(buf);
    //uint16_t appid = asUint16(buf+2);
    uint16_t serviceId = asUint16(buf+4);
    uint16_t commandId = asUint16(buf+6);
    g_seqNum = asUint16(buf+8);

    LOG_DEBUG << "serviceId=" << serviceId;
    LOG_DEBUG << "commandId=" << commandId;

    std::string typeName = idToTypeName(serviceId, commandId);
    if (typeName != "unknow") {
        message.reset(createMessage(typeName));
        if (message)
        {
            // parse from buffer
            const char* data = buf + 12;
            int32_t dataLen = len - kHeaderLen2 + 4;
            if (message->ParseFromArray(data, dataLen))
            {
                *error = kNoError;
            }
            else
            {
                *error = kParseError;
            }
        }
    } else {
        *error = kUnknownMessageType;
    }
    
    return message;
}

std::string ProtobufCodec::idToTypeName(uint16_t serviceId, uint16_t commandId)
{
    std::string typeName;
    switch (commandId)
    {
    case 259:
        typeName = "IM.Login.IMLoginReq";
        break;
    case 260:
        typeName = "IM.Login.IMLoginRes";
        break;
    case 261:
        typeName = "IM.Login.IMLogoutRes";
        break;
    case 262:
        typeName = "IM.Login.IMLogoutRsp";
        break;
    case 513:
        typeName = "IM.Buddy.IMRecentContactSessionReq";
        break;
    case 520:
        typeName = "IM.Buddy.IMAllUserReq";
        break;
    case 522:
        typeName = "IM.Buddy.IMUsersStatReq";
        break;
    case 528:
        typeName = "IM.Buddy.IMDepartmentReq";
        break;
    case 1025:
        typeName = "IM.Group.IMNormalGroupListReq";
        break;
    case 1793:
        typeName = "IM.Other.IMHeartBeat";
        break;
    default:
        typeName = "unknow";
        break;
    }

    return typeName;
}

std::pair<uint16_t, uint16_t> ProtobufCodec::typeNameToId(std::string typeName)
{
    std::pair<uint16_t, uint16_t> id;
    uint16_t serviceId;
    uint16_t commandId;
    
    if (typeName == "IM.Login.IMLoginRes") {
        serviceId = 1;
        commandId = 260;
    } else if (typeName == "IM.Login.IMLoginReq") {
        serviceId = 1;
        commandId = 259;
    } else if (typeName == "IM.Login.IMLogoutRsp") {
        serviceId = 1;
        commandId = 262;
    } else if (typeName == "IM.Buddy.IMDepartmentRsp") {
        serviceId = 2;
        commandId = 529;
    } else if (typeName == "IM.Buddy.IMAllUserRsp") {
        serviceId = 2;
        commandId = 521;
    } else if (typeName == "IM.Other.IMHeartBeat") {
        serviceId = 7;
        commandId = 1793;
    } else {
        return id;
    }
    
    return std::pair<uint16_t, uint16_t>(serviceId, commandId);
}