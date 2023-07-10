//
// Created by curtis on 7/6/23.
//



#include "fast_reconfig.h"

namespace Envoy {
namespace Server {

using GrpcMessageImpl = FastReconfigServerImpl::GrpcMessageImpl;
using GrpcRequestProcessorImpl = FastReconfigServerImpl::GrpcRequestProcessorImpl;

FastReconfigServerImpl::FastReconfigServerImpl(Server::Instance& server,
                                               LdsApiImpl& listener_reconfig_callback)
    : server_(server),
      listener_reconfig_handler_instance_(server_, listener_reconfig_callback)
                                               {
  handler_registry_["/rr_listener"] =
      registerHandler(listener_reconfig_handler_instance_.pushNewListenersHandler);
}

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

GrpcRequestProcessorPtr FastReconfigServerImpl::matchAndApplyFactoryOnRequest(AdminStream &admin_stream) const {
  // get request path
  // fetch factory from registry
  // apply factory with admin_stream, and return the processor.
  auto iter = handler_registry_.find("/rr_listener");
  if (iter != handler_registry_.end()) {
    return iter->processor_factory_(admin_stream);
  } else {

  }
}

bool FastReconfigServerImpl::createFilterChain(Http::FilterChainManager &manager, bool) const {
  Http::FilterFactoryCb factory = [this](Http::FilterChainFactoryCallbacks& callbacks) {
    callbacks.addStreamFilter(std::make_shared<FastReconfigFilter>(
        [this](AdminStream& admin_stream) { return matchAndApplyFactoryOnRequest(admin_stream); }
        ));
  };
  manager.applyFilterFactoryCb({}, factory);
  return true;
}

}
}