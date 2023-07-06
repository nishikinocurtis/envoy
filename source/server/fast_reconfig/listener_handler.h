//
// Created by qiutong on 7/6/23.
//

#include "reconfig_handler_base.h"

namespace Envoy {
namespace Server {

class ListenerHandler : public ReconfigHandlerBase {
public:
    ListenerHandler(Server::Instance& server, Config::SubscriptionCallbacks& callbacks)
        : ReconfigHandlerBase(server, callbacks) {};

    Http::Code pushNewListenersHandler(AdminStream& admin_stream,
                                       Http::ResponseHeaderMap& response_headers,
                                       Buffer::Instance& response);
};

}
}
