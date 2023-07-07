//
// Created by qiutong on 7/6/23.
//

#include "envoy/config/subscription.h"


#include "reconfig_handler_base.h"
#include "source/common/common/logger.h"
#include "source/extensions/listener_managers/listener_manager/lds_api.h"
#include "source/common/config/subscription_base.h"



namespace Envoy {
namespace Server {

using LdsApiImplPtr = std::shared_ptr<LdsApiImpl>;

class ListenerHandler : public ReconfigHandlerBase,
                        Logger::Loggable<Logger::Id::rr_manager> {
public:
    ListenerHandler(Server::Instance& server,
                    LdsApiImpl& lds_handle
                    )
        : ReconfigHandlerBase(server, lds_handle), listener_sub_handle_(lds_handle) {};

    Http::Code pushNewListenersHandler(AdminStream& admin_stream,
                                       Http::ResponseHeaderMap& response_headers,
                                       Buffer::Instance& response);
private:
  const LdsApiImpl& listener_sub_handle_;
};

}
}
