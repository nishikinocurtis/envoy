//
// Created by curtis on 7/1/23.
//

#pragma once

#include "envoy/buffer/buffer.h"
#include "envoy/common/pure.h"
#include "envoy/grpc/status.h"
#include "envoy/http/header_map.h"
#include "envoy/tracing/http_tracer.h"

#include "source/common/common/assert.h"
#include "source/common/protobuf/protobuf.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Grpc {

class RawAsyncListeningStream {

};

class RawAsyncGrpcHandlerCallbacks {
public:
  virtual ~RawAsyncGrpcHandlerCallbacks() = default;


};

class RawAsyncServer {
public:
  virtual ~RawAsyncServer() = default;

  /**
   * Expose the gRPC endpoint, adopted from envoy/server/admin.h.
   * @param access_logs
   * @param address_out_path
   * @param address
   * @param socket_options
   * @param listener_scope
   */
  virtual void startServing(const std::list<AccessLog::InstanceSharedPtr>& access_logs,
                            const std::string& address_out_path,
                            Network::Address::InstanceConstSharedPtr address,
                            const Network::Socket::OptionsSharedPtr& socket_options,
                            Stats::ScopeSharedPtr&& listener_scope) PURE;
};

} // namespace Grpc
} // namespace Envoy
