//
// Created by qiutong on 7/31/23.
//

#pragma once

#include "envoy/config/subscription.h"

#include "reconfig_handler_base.h"
#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"

namespace Envoy {
namespace Server {

class ClusterHandler : public ReconfigHandlerBase,
                       Logger::Loggable<Logger::Id::rr_manager> {
public:
  ClusterHandler(Server::Instance& server,
                 SubscriptionCallbackWeakPtr&& cds_handle,
                 Config::OpaqueResourceDecoderSharedPtr&& cds_resource_decoder)
                 : ReconfigHandlerBase(server, std::move(cds_handle)), resource_decoder_(cds_resource_decoder) {}

  Http::Code pushNewClusterInfo(AdminStream& admin_stream,
                                Http::ResponseHeaderMap& response_headers,
                                Buffer::Instance& response);
private:
  Config::OpaqueResourceDecoderSharedPtr resource_decoder_;
};

}
}
