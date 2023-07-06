//
// Created by qiutong on 7/6/23.
//

#include "listener_handler.h"

namespace Envoy {
namespace Server {

Http::Code ListenerHandler::pushNewListenersHandler(Envoy::Server::AdminStream &admin_stream,
                                                            Http::ResponseHeaderMap &response_headers,
                                                            Buffer::Instance &response) {
    // copy out request body from admin_stream as bytes
    // pass to protobuf CodedInputStream
    // decode to protobuf Message
    // extract version info from header
    // pass to callback_.onConfigUpdate()
    // return Ok,
    // return non-ok on exception
}

}
}