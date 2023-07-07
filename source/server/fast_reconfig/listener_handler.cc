//
// Created by qiutong on 7/6/23.
//

#include "listener_handler.h"
#include "source/common/protobuf/protobuf.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "source/common/common/assert.h"
#include "source/common/config/decoded_resource_impl.h"


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
    auto siz_ = admin_stream.getRequestBody()->length();
    auto raw_body = std::unique_ptr<uint8_t[]>(new uint8_t[siz_]);
    admin_stream.getRequestBody()->copyOut(0, siz_, raw_body.get());

    auto pb_stream = Protobuf::CodedInputStream(raw_body.get(), siz_);
    // create a message (DiscoveryResponse) or anything, create a decoded_resource from the message:
    // using DecodedResourcesWrapper(*resource_decoder_, message.resources(), message.version_info());
    // then pass to lds_api.onConfigUpdate, with version_info included.
    envoy::service::discovery::v3::DeltaDiscoveryResponse ddr;
    auto success = ddr.ParseFromCodedStream(pb_stream);
    if (!success) {
        ENVOY_LOG(debug, "pushing listener parsing failed");
        return Http::Code::BadRequest;
    }

    auto added_resources =
      Config::SpecifiedResourcesWrapper<envoy::service::discovery::v3::Resource>
        (*resource_decoder_, ddr.resources(), ddr.system_version_info());

    callback_.onConfigUpdate(added_resources.refvec_,
                             ddr.removed_resources(),
                             ddr.system_version_info());
    // write response headers
    // write response
    return Http::Code::OK;

}

}
}