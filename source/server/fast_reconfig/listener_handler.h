//
// Created by qiutong on 7/6/23.
//

#include "reconfig_handler_base.h"
#include "source/common/common/logger.h"
#include "envoy/config/subscription.h"

namespace Envoy {
namespace Server {

class ListenerHandler : public ReconfigHandlerBase,
                        Logger::Loggable<Logger::Id::rr_manager> {
public:
    ListenerHandler(Server::Instance& server,
                    Config::SubscriptionCallbacks& callbacks,
                    Config::OpaqueResourceDecoderSharedPtr resource_decoder)
        : ReconfigHandlerBase(server, callbacks), resource_decoder_(resource_decoder) {};

    Http::Code pushNewListenersHandler(AdminStream& admin_stream,
                                       Http::ResponseHeaderMap& response_headers,
                                       Buffer::Instance& response);
private:
    Config::OpaqueResourceDecoderSharedPtr resource_decoder_;
};

}
}
