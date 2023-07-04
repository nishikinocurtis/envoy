//
// Created by curtis on 7/5/23.
//

#pragma once

#include <functional>
#include <list>

#include "envoy/http/filter.h"
#include "envoy/server/admin.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
#include "source/common/http/codes.h"
#include "source/common/http/header_map_impl.h"
#include "source/extensions/filters/http/common/pass_through_filter.h"

#include "source/server/server_endpoint_filter.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Server {

class GrpcRequestPtr;
/*
 * A gRPC filter that implements re-config fast pushing endpoint functionality.
 * Adopted from Envoy::Server::AdminFilter.
 */
class FastReconfigFilter : public ServerEndpointFilterBase,
                           Logger::Loggable<Logger::Id::rr_manager> {

public:
  using GenRequestFn = std::function<GrpcRequestPtr(AdminStream&)>;
  FastReconfigFilter(GenRequestFn fast_reconfig_handler_func);

  // AdminStream, override for logging.
  Http::StreamDecoderFilterCallbacks& getDecoderFilterCallbacks() const override;
  const Http::RequestHeaderMap& getRequestHeaders() const override;

private:
  /**
   * Called when an admin request has been completely received.
   */
  void onComplete() override;
  GenRequestFn server_handler_fn_;
};

}
}
