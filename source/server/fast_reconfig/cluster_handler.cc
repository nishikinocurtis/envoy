//
// Created by qiutong on 7/31/23.
//

#include "cluster_handler.h"
#include "source/common/protobuf/protobuf.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "source/common/common/assert.h"
#include "source/common/config/decoded_resource_impl.h"

namespace Envoy {
namespace Server {

Http::Code ClusterHandler::pushNewClusterInfo(AdminStream &admin_stream, Http::ResponseHeaderMap &response_headers,
                                              Buffer::Instance &response) {
  ENVOY_LOG(debug, "entering pushNewClusterInfo processor");

  auto siz_ = admin_stream.getRequestBody()->length();
  auto raw_body = std::unique_ptr<uint8_t[]>(new uint8_t[siz_]);
  admin_stream.getRequestBody()->copyOut(0, siz_, raw_body.get());

  auto pb_stream = std::make_unique<Protobuf::io::CodedInputStream>(raw_body.get(), siz_);

  envoy::service::discovery::v3::DiscoveryResponse ddr;
  auto success = ddr.ParseFromCodedStream(pb_stream.get());
  if (!success) {
    ENVOY_LOG(debug, "pushing cluster parsing failed");
    return Http::Code::BadRequest;
  }
  // note: for delta response, the resources field is of type Resource,
  // so use SpecifiedDecodedResourcesWrapper instead.
  // also, remember to replace the onConfigUpdate() to delta version.
  auto resources =
      Config::DecodedResourcesWrapper
          (*resource_decoder_, ddr.resources(), ddr.version_info());

  if (!callback_.expired()) {
    callback_.lock()->onConfigUpdate(resources.refvec_,
                                     ddr.version_info());
  }

  // write response headers
  response_headers.setCopy(Http::LowerCaseString("x-ftmesh-cluster"), "true");
  // write response
  response.add("\r\n");
  return Http::Code::OK;
}

}
}