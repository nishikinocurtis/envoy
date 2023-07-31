//
// Created by qiutong on 7/6/23.
//
#pragma once

#include "envoy/config/subscription.h"

#include "reconfig_handler_base.h"
#include "source/common/common/logger.h"
#include "source/extensions/listener_managers/listener_manager/lds_api.h"
#include "source/common/config/subscription_base.h"



namespace Envoy {
namespace Server {

// we need something that is a SubscriptionCallback and can get resource_decoder from.

class ListenerHandler : public ReconfigHandlerBase,
                        Logger::Loggable<Logger::Id::rr_manager> {
public:
    // TODO: consider using a weak_ptr
    ListenerHandler(Server::Instance& server,
                    SubscriptionCallbackWeakPtr&& lds_handle,
                    Config::OpaqueResourceDecoderSharedPtr&& lds_resource_decoder
                    // Config::OpaqueResourceDecoderSharedPtr&& resource_decoder
                    )
        : ReconfigHandlerBase(server, std::move(lds_handle)), resource_decoder_(std::move(lds_resource_decoder)) {};

    Http::Code pushNewListenersHandler(AdminStream& admin_stream,
                                       Http::ResponseHeaderMap& response_headers,
                                       Buffer::Instance& response);
private:
  // const LdsApiImpl& listener_sub_handle_;
  Config::OpaqueResourceDecoderSharedPtr resource_decoder_;
};

}
}
