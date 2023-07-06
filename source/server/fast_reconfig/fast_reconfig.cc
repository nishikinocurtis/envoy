//
// Created by curtis on 7/6/23.
//



#include "fast_reconfig.h"

namespace Envoy {
namespace Server {

using GrpcMessageImpl = FastReconfigServerImpl::GrpcMessageImpl;
using GrpcRequestProcessorImpl = FastReconfigServerImpl::GrpcRequestProcessorImpl;

GrpcMessageImpl::GrpcMessageImpl() {
  headers_ = Http::ResponseHeaderMapImpl::create();
  buf_ = std::make_unique<Buffer::OwnedImpl>();
}

std::pair<bool, std::unique_ptr<Buffer::Instance>> GrpcMessageImpl::fetchNextMessageBodyChunk() {
  return std::make_pair(false, std::move(buf_));
}

GrpcRequestProcessorImpl::
    GrpcRequestProcessorImpl(FastReconfigServer::HandlerCb handler_callback,
                             AdminStream& admin_stream)
    : handler_cb_(handler_callback),
      admin_stream_(admin_stream),
      response_msg_(std::make_unique<GrpcMessageImpl>()) {}

FastReconfigServer::GrpcMessageOverHttpPtr GrpcRequestProcessorImpl::process() {
  response_msg_->code_ =
      handler_cb_(admin_stream_, *response_msg_->headers_, *response_msg_->buf_);
  return std::move(response_msg_);
}

}

}