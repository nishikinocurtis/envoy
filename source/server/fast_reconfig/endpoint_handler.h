//
// Created by qiutong on 1/11/24.
//

#pragma once

#include "envoy/config/subscription.h"

#include "source/common/common/logger.h"
#include "source/extensions/clusters/eds/eds.h"

namespace Envoy {
namespace Server {

class EndpointReconfigHandler : Logger::Loggable<Logger::Id::rr_manager> {
public:
  EndpointReconfigHandler() : eds_manager_(Upstream::EndpointClusterReroutingManager::get()) {}
  static Http::Code pushNewEndpointInfo(AdminStream& admin_stream,
                                 Http::ResponseHeaderMap& response_headers,
                                 Buffer::Instance& response);
private:
  Upstream::EndpointClusterReroutingManager& eds_manager_;
};

} // namespace Server
} // namespace Envoy
