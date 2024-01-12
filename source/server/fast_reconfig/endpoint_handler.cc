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
                                                        Http::ResponseHeaderMap &,
                                                        Buffer::Instance &) {
  ENVOY_LOG(debug, "entering pushNewEndpointInfo processor");

  auto siz_ = admin_stream.getRequestBody()->length();
  auto raw_body = std::unique_ptr<char>(new char[siz_]);
  admin_stream.getRequestBody()->copyOut(0, siz_, raw_body.get());

  // auto pb_stream = std::make_unique<Protobuf::io::CodedInputStream>(raw_body.get(), siz_);
  auto request_str = std::string{*raw_body};
  auto metadata = [&request_str]() -> std::vector<std::string> {
    std::vector<std::string> result;
    std::istringstream iss(request_str);
    std::string line;

    while (std::getline(iss, line)) {
      result.push_back(line);
    }

    return result;
  }();

  auto edsHandle = Upstream::EndpointClusterReroutingManager::get().fetchEdsHandleByCluster(metadata[0]);
  if (edsHandle != nullptr) {
    return edsHandle->replaceHost(metadata[1], std::stoul(metadata[2]), metadata[3], std::stoul(metadata[4]))
              ? Http::Code::OK : Http::Code::InternalServerError;
  } else {
    return Http::Code::BadRequest;
  }
}

} // namespace Server
} // namespace Envoy