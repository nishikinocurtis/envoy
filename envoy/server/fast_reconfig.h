//
// Created by curtis on 6/30/23.
//

#pragma once

#include <functional>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/pure.h"
#include "envoy/grpc/context.h"
#include "envoy/config/subscription.h"
#include "envoy/server/admin.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Server {

/**
 * A listening grpc server abstraction for fast update of listener configuration,
 * which bypasses the normal update->control plane->notification path.
 * However it aims at being compatible with xDS subscriptions, by mutating
 * the hash value and version_info.
 */

class FastReconfigServer : public Config::SubscriptionCallbacks {
public:
  /*
   * Provide a fake stream manner GrpcMessage for one time
   * Reconfig Processing
   */
  class GrpcMessageOverHttp {
  public:
    virtual ~GrpcMessageOverHttp() = default;

    virtual Http::Code getMessageStatus() PURE;
    virtual std::unique_ptr<Http::ResponseHeaderMap> dumpMessageHeader() PURE;
    virtual std::pair<bool, std::unique_ptr<Buffer::Instance>> fetchNextMessageBodyChunk() PURE;
  };
  using GrpcMessageOverHttpPtr = std::unique_ptr<GrpcMessageOverHttp>;

  /*
   * Wrapper for a GrpcRequest Processor functor, generate a
   * GrpcMessageOverHttp for stream manner encoder compatibility.
   * */
  class GrpcRequestProcessor {
  public:
    virtual ~GrpcRequestProcessor() = default;

    virtual GrpcMessageOverHttpPtr process() PURE;


  };

  using HandlerCb =
      std::function<Http::Code(AdminStream& admin_stream, Http::ResponseHeaderMap& response_headers,
                               Buffer::Instance& response)>;

  using GrpcRequestProcessorPtr = std::unique_ptr<GrpcRequestProcessor>;
};


} // namespace Server
} // namespace Envoy

