//
// Created by curtis on 6/30/23.
//

#pragma once

#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/server/listener_manager.h"
#include "envoy/network/listen_socket.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/buffer/buffer_impl.h"

#include "source/common/common/logger.h"

namespace Envoy {
namespace Server {

class FastReconfigServerImpl : public FastReconfigServer,
                               Logger::Loggable<Logger::Id::rr_manager> {
public:
  class GrpcMessageImpl;

  class GrpcRequestProcessorImpl : public GrpcRequestProcessor {
  public:
    GrpcRequestProcessorImpl(HandlerCb handler_callback, AdminStream& admin_stream);

    GrpcMessageOverHttpPtr process() override;

  private:
    HandlerCb handler_cb_;
    AdminStream& admin_stream_;
    std::unique_ptr<GrpcMessageImpl> response_msg_;
  };

  class GrpcMessageImpl : public GrpcMessageOverHttp {
  public:
    GrpcMessageImpl();
    // Http::Code writeMessageStatus(Http::Code new_code) { code_ = new_code; };
    Http::Code getMessageStatus() override { return code_; };

    // should only be executed once
    std::unique_ptr<Http::ResponseHeaderMap> dumpMessageHeader() override { return std::move(headers_); } ;
    std::pair<bool, std::unique_ptr<Buffer::Instance>> fetchNextMessageBodyChunk() override;

  private:
    friend class GrpcRequestProcessorImpl;
    std::unique_ptr<Http::ResponseHeaderMapImpl> headers_;
    std::unique_ptr<Buffer::OwnedImpl> buf_;
    Http::Code code_ = Http::Code::OK;
  };


};

} // namespace Server
} // namespace Envoy
