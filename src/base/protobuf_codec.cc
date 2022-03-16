// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "protobuf_codec.h"

#include "pbs/IM.BaseDefine.pb.h"
#include "slite/Logger.h"
#include "google-inl.h"

#include <google/protobuf/descriptor.h>

#include <zlib.h>  // adler32
#include <string>
#include <arpa/inet.h>

using namespace slite;
using namespace IM;

static uint16_t g_seqNum;

void ProtobufCodec::fillEmptyBuffer(std::string& buf, const google::protobuf::Message& message)
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

  uint8_t* start = reinterpret_cast<uint8_t*>(buf.data() + kHeaderLen - 4);
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
    if (conn && conn->connected()) {
        conn->shutdown();
    }
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
    while (buf.size() >= kMinMessageLen + kHeaderLen) {
        int32_t len;
        ::memcpy(&len, buf.data(), sizeof len);
        len = ntohl(len);
        if (len > kMaxMessageLen || len < kMinMessageLen) {
            errorCallback_(conn, buf, receiveTime, kInvalidLength);
            break;
        } else if (buf.size() >= static_cast<size_t>(len)) {
            ErrorCode errorCode = kNoError;
            MessagePtr message = parse(buf.data()+4, len-4, &errorCode);
            if (errorCode == kNoError && message) {
                messageCallback_(conn, message, receiveTime);
                buf = buf.substr(len, buf.size()-len);
            } else {
                errorCallback_(conn, buf, receiveTime, errorCode);
                break;
            }
        } else {
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

    uint16_t version = asUint16(buf);
    uint16_t appid = asUint16(buf+2);
    uint16_t serviceId = asUint16(buf+4);
    uint16_t commandId = asUint16(buf+6);
    g_seqNum = asUint16(buf+8);

    std::string typeName = idToTypeName(serviceId, commandId);

    if (typeName != "unknow") {
        message.reset(createMessage(typeName));
        if (message) {
            // parse from buffer
            const char* data = buf + 12;
            int32_t dataLen = len - kHeaderLen + 4;
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
        LOG_DEBUG << "version=" << version;
        LOG_DEBUG << "appid=" << appid;
        LOG_DEBUG << "serviceId=" << serviceId;
        LOG_DEBUG << "commandId=" << commandId;
        LOG_DEBUG << "typeName=" << typeName;
        LOG_DEBUG << "seqNum=" << g_seqNum;
        *error = kUnknownMessageType;
    }
    
    return message;
}

std::string ProtobufCodec::idToTypeName(uint16_t serviceId, uint16_t commandId)
{
    std::string typeName;
    switch (commandId)
    {
    case IM::BaseDefine::CID_LOGIN_REQ_USERLOGIN:
        typeName = "IM.Login.IMLoginReq";
        break;
    case IM::BaseDefine::CID_LOGIN_RES_USERLOGIN:
        typeName = "IM.Login.IMLoginRes";
        break;
    case IM::BaseDefine::CID_LOGIN_REQ_LOGINOUT:
        typeName = "IM.Login.IMLogoutReq";
        break;
    case IM::BaseDefine::CID_LOGIN_RES_LOGINOUT:
        typeName = "IM.Login.IMLogoutRes";
        break;
    case IM::BaseDefine::CID_BUDDY_LIST_RECENT_CONTACT_SESSION_REQUEST:
        typeName = "IM.Buddy.IMRecentContactSessionReq";
        break;
    case IM::BaseDefine::CID_BUDDY_LIST_ALL_USER_REQUEST:
        typeName = "IM.Buddy.IMAllUserReq";
        break;
    case IM::BaseDefine::CID_BUDDY_LIST_USERS_STATUS_REQUEST:
        typeName = "IM.Buddy.IMUsersStatReq";
        break;
    case IM::BaseDefine::CID_BUDDY_LIST_DEPARTMENT_REQUEST:
        typeName = "IM.Buddy.IMDepartmentReq";
        break;
    case IM::BaseDefine::CID_MSG_DATA:
        typeName = "IM.Message.IMMsgData";
        break;
    case IM::BaseDefine::CID_MSG_DATA_ACK:
        typeName = "IM.Message.IMMsgDataAck";
        break;
    case IM::BaseDefine::CID_MSG_READ_ACK:
        typeName = "IM.Message.IMMsgDataReadAck";
        break;
    case IM::BaseDefine::CID_MSG_UNREAD_CNT_REQUEST:
        typeName = "IM.Message.IMUnreadMsgCntReq";
        break;
    case IM::BaseDefine::CID_MSG_LIST_REQUEST:
        typeName = "IM.Message.IMGetMsgListReq";
        break;
    case IM::BaseDefine::CID_GROUP_NORMAL_LIST_REQUEST:
        typeName = "IM.Group.IMNormalGroupListReq";
        break;
    case IM::BaseDefine::CID_FILE_HAS_OFFLINE_REQ:
        typeName = "IM.File.IMFileHasOfflineReq";
        break;
    case IM::BaseDefine::CID_SWITCH_P2P_CMD:
        typeName = "IM.SwitchService.IMP2PCmdMsg";
        break;
    case IM::BaseDefine::CID_OTHER_HEARTBEAT:
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
        commandId = IM::BaseDefine::CID_LOGIN_RES_USERLOGIN;
    } else if (typeName == "IM.Login.IMKickUser") {
        serviceId = 1;
        commandId = IM::BaseDefine::CID_LOGIN_KICK_USER;
    } else if (typeName == "IM.Login.IMLoginReq") {
        serviceId = 1;
        commandId = IM::BaseDefine::CID_LOGIN_REQ_USERLOGIN;
    } else if (typeName == "IM.Login.IMLogoutRsp") {
        serviceId = 1;
        commandId = IM::BaseDefine::CID_LOGIN_RES_LOGINOUT;
    } else if (typeName == "IM.Buddy.IMDepartmentRsp") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_DEPARTMENT_RESPONSE;
    } else if (typeName == "IM.Buddy.IMAllUserRsp") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_ALL_USER_RESPONSE;
    } else if (typeName == "IM.Buddy.IMRecentContactSessionRsp") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_RECENT_CONTACT_SESSION_RESPONSE;
    } else if (typeName == "IM.Buddy.IMUsersStatRsp") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_USERS_STATUS_RESPONSE;
    } else if (typeName == "IM.Buddy.IMUserStatNotify") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_STATUS_NOTIFY;
    } else if (typeName == "IM.Buddy.IMRemoveSessionNotify") {
        serviceId = 2;
        commandId = IM::BaseDefine::CID_BUDDY_LIST_REMOVE_SESSION_NOTIFY;
    } else if (typeName == "IM.Message.IMUnreadMsgCntRsp") {
        serviceId = 3;
        commandId = IM::BaseDefine::CID_MSG_UNREAD_CNT_RESPONSE;
    } else if (typeName == "IM.Message.IMMsgData") {
        serviceId = 3;
        commandId = IM::BaseDefine::CID_MSG_DATA;
    } else if (typeName == "IM.Message.IMGetMsgListRsp") {
        serviceId = 3;
        commandId = IM::BaseDefine::CID_MSG_LIST_RESPONSE;
    } else if (typeName == "IM.Message.IMMsgDataAck") {
        serviceId = 3;
        commandId = IM::BaseDefine::CID_MSG_DATA_ACK;
    } else if (typeName == "IM.Message.IMMsgDataReadNotify") {
        serviceId = 3;
        commandId = IM::BaseDefine::CID_MSG_READ_NOTIFY;
    } else if (typeName == "IM.Group.IMNormalGroupListRsp") {
        serviceId = 4;
        commandId = IM::BaseDefine::CID_GROUP_NORMAL_LIST_RESPONSE;
    } else if (typeName == "IM.SwitchService.IMP2PCmdMsg") {
        serviceId = 6;
        commandId = IM::BaseDefine::CID_SWITCH_P2P_CMD;
    } else if (typeName == "IM.Other.IMHeartBeat") {
        serviceId = 7;
        commandId = IM::BaseDefine::CID_OTHER_HEARTBEAT;
    } else {
        return id;
    }
    
    return std::pair<uint16_t, uint16_t>(serviceId, commandId);
}