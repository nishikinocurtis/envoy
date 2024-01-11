//
// Created by qiutong on 1/11/24.
//

#pragma onceonce

#include "envoy/config/subscription.h"

#include "reconfig_handler_base.h"
#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"

namespace Envoy {
namespace Server {

class EndpointReconfigHandler : public ReconfigHandlerBase,
                                Logger::Loggable<Logger::Id::rr_manager> {
public:
  EndpointReconfigHandler(Server::Instance& server,
                          SubscriptionCallbackWeakPtr&& eds_handle,
                          Config::OpaqueResourceDecoderSharedPtr&& eds_resource_decoder)
                          : ReconfigHandlerBase(server, std::move(eds_handle)), resource_decoder_(eds_resource_decoder) {}
  Http::Code pushNewEndpointInfo(AdminStream& admin_stream,
                                 Http::ResponseHeaderMap& response_headers,
                                 Buffer::Instance& response);
private:
  Config::OpaqueResourceDecoderSharedPtr resource_decoder_;
};

} // namespace Server
} // namespace Envoy
