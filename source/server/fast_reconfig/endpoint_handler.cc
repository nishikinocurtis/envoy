//
// Created by qiutong on 1/11/24.
//

#include "endpoint_handler.h"
#include "source/common/protobuf/protobuf.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "source/common/common/assert.h"
#include "source/common/config/decoded_resource_impl.h"

namespace Envoy {
namespace Server {

Http::Code EndpointReconfigHandler::pushNewEndpointInfo(Envoy::Server::AdminStream &admin_stream,
                                                        Http::ResponseHeaderMap &response_headers,
                                                        Buffer::Instance &response) {
  ENVOY_LOG(debug, "entering pushNewEndpointInfo processor");

  auto siz_ = admin_stream.getRequestBody()->length();
  auto raw_body = std::unique_ptr<char>(new char[siz_]);
  admin_stream.getRequestBody()->copyOut(0, siz_, raw_body.get());

  // auto pb_stream = std::make_unique<Protobuf::io::CodedInputStream>(raw_body.get(), siz_);


}

} // namespace Server
} // namespace Envoy