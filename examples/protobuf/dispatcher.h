// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_EXAMPLES_PROTOBUF_CODEC_DISPATCHER_H
#define MUDUO_EXAMPLES_PROTOBUF_CODEC_DISPATCHER_H

#include "TCPConnection.h"

#include <google/protobuf/message.h>

#include <map>

#include <type_traits>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

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

class Callback
{
 public:
  virtual ~Callback() = default;
  virtual void onMessage(const slite::TCPConnectionPtr&,
                         const MessagePtr& message,
                         int64_t) const = 0;
};

template <typename T>
class CallbackT : public Callback
{
  static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                "T must be derived from gpb::Message.");
 public:
  typedef std::function<void (const slite::TCPConnectionPtr&,
                                const std::shared_ptr<T>& message,
                                int64_t)> ProtobufMessageTCallback;

  CallbackT(const ProtobufMessageTCallback& callback)
    : callback_(callback)
  {
  }

  void onMessage(const slite::TCPConnectionPtr& conn,
                 const MessagePtr& message,
                 int64_t receiveTime) const override
  {
    std::shared_ptr<T> concrete = down_pointer_cast<T>(message);
    assert(concrete != NULL);
    callback_(conn, concrete, receiveTime);
  }

 private:
  ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher
{
 public:
  typedef std::function<void (const slite::TCPConnectionPtr&,
                                const MessagePtr& message,
                                int64_t receiveTime)> ProtobufMessageCallback;

  explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
    : defaultCallback_(defaultCb)
  {
  }

  void onProtobufMessage(const slite::TCPConnectionPtr& conn,
                         const MessagePtr& message,
                         int64_t receiveTime) const
  {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end())
    {
      it->second->onMessage(conn, message, receiveTime);
    }
    else
    {
      defaultCallback_(conn, message, receiveTime);
    }
  }

  template<typename T>
  void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
  {
    std::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
    callbacks_[T::descriptor()] = pd;
  }

 private:
  typedef std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback> > CallbackMap;

  CallbackMap callbacks_;
  ProtobufMessageCallback defaultCallback_;
};
#endif  // MUDUO_EXAMPLES_PROTOBUF_CODEC_DISPATCHER_H

