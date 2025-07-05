// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "slite/protorpc/RpcChannel.h"

#include "slite/Logger.h"
#include "slite/protorpc/rpc.pb.h"

#include <google/protobuf/descriptor.h>

using namespace slite;
// using namespace slite::protorpc;
using namespace std::placeholders;

RpcChannel::RpcChannel()
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      services_(NULL)
{
  LOG_DEBUG << "RpcChannel::ctor - " << this;
}

RpcChannel::RpcChannel(const TCPConnectionPtr &conn)
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      conn_(conn),
      services_(NULL)
{
  LOG_DEBUG << "RpcChannel::ctor - " << this;
}

RpcChannel::~RpcChannel()
{
  LOG_DEBUG << "RpcChannel::dtor - " << this;
  for (const auto &outstanding : outstandings_) {
    OutstandingCall out = outstanding.second;
    delete out.response;
    delete out.done;
  }
}

// Call the given method of the remote service.  The signature of this
// procedure looks the same as Service::CallMethod(), but the requirements
// are less strict in one important way:  the request and response objects
// need not be of any specific class as long as their descriptors are
// method->input_type() and method->output_type().
void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const ::google::protobuf::Message *request,
                            ::google::protobuf::Message *response,
                            ::google::protobuf::Closure *done)
{
  RpcMessage message;
  message.set_type(REQUEST);
  int64_t id = id_++;
  message.set_id(id);
  message.set_service(method->service()->full_name());
  message.set_method(method->name());
  message.set_request(request->SerializeAsString()); // FIXME: error check

  OutstandingCall out = {response, done};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    outstandings_[id] = out;
  }

  client_->setConnectionCallback(std::bind(&RpcChannel::connectedCallback, this, _1, message));
  client_->setMessageCallback(std::bind(&RpcChannel::onMessage, this, _1, _2, _3));
  client_->connect();
  loop_->loop();
}

void RpcChannel::onMessage(const TCPConnectionPtr &conn,
                           std::string buf,
                           int64_t receiveTime)
{
  codec_.onMessage(conn, buf, receiveTime);
}

void RpcChannel::onRpcMessage(const TCPConnectionPtr &conn,
                              const RpcMessagePtr &messagePtr,
                              int64_t receiveTime)
{
  assert(conn == conn_);
  // printf("%s\n", message.DebugString().c_str());
  RpcMessage &message = *messagePtr;
  if (message.type() == RESPONSE) {
    int64_t id = message.id();
    assert(message.has_response() || message.has_error());

    OutstandingCall out = {NULL, NULL};

    {
      std::lock_guard<std::mutex> lock(mutex_);
      std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(id);
      if (it != outstandings_.end()) {
        out = it->second;
        outstandings_.erase(it);
      }
    }

    if (out.response) {
      std::unique_ptr<google::protobuf::Message> d(out.response);
      if (message.has_response()) {
        out.response->ParseFromString(message.response());
      }
      if (out.done) {
        out.done->Run();
        client_->disconnect();
        loop_->quit();
      }
    }
  } else if (message.type() == REQUEST) {
    // FIXME: extract to a function
    ErrorCode error = WRONG_PROTO;
    if (services_) {
      std::map<std::string, google::protobuf::Service *>::const_iterator it = services_->find(message.service());
      if (it != services_->end()) {
        google::protobuf::Service *service = it->second;
        assert(service != NULL);
        const google::protobuf::ServiceDescriptor *desc = service->GetDescriptor();
        const google::protobuf::MethodDescriptor *method = desc->FindMethodByName(message.method());
        if (method) {
          std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
          if (request->ParseFromString(message.request())) {
            google::protobuf::Message *response = service->GetResponsePrototype(method).New();
            // response is deleted in doneCallback
            int64_t id = message.id();
            service->CallMethod(method, NULL, request.get(), response,
                                NewCallback(this, &RpcChannel::doneCallback, response, id));
            error = NO_ERROR;
          } else {
            error = INVALID_REQUEST;
          }
        } else {
          error = NO_METHOD;
        }
      } else {
        error = NO_SERVICE;
      }
    } else {
      error = NO_SERVICE;
    }
    if (error != NO_ERROR) {
      RpcMessage response;
      response.set_type(RESPONSE);
      response.set_id(message.id());
      response.set_error(error);
      codec_.send(conn_, response);
    }
  } else if (message.type() == ERROR) {
  }
}

void RpcChannel::doneCallback(::google::protobuf::Message *response, int64_t id)
{
  std::unique_ptr<google::protobuf::Message> d(response);
  slite::RpcMessage message;
  message.set_type(RESPONSE);
  message.set_id(id);
  message.set_response(response->SerializeAsString()); // FIXME: error check
  codec_.send(conn_, message);
}

void RpcChannel::connectedCallback(const TCPConnectionPtr& conn, const RpcMessage& request)
{
    if (conn->connected()) {
        codec_.send(conn, request);
        this->conn_ = conn;
    }
}