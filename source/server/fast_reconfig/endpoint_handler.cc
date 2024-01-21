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
#ifdef TIME_EVAL
  auto enter_now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  printf("entering endpoint handler: %lld\n", enter_now);
#endif
  ENVOY_LOG(debug, "entering pushNewEndpointInfo processor");
  printf("matched and entering new endpoint\n");

  auto siz_ = admin_stream.getRequestBody()->length();
  auto raw_body = std::unique_ptr<char[]>(new char[siz_]);
  admin_stream.getRequestBody()->copyOut(0, siz_, raw_body.get());

  // auto pb_stream = std::make_unique<Protobuf::io::CodedInputStream>(raw_body.get(), siz_);
  std::string request_str(raw_body.get());
  auto metadata = [&request_str]() -> std::vector<std::string> {
    std::vector<std::string> result;
    std::istringstream iss(request_str);
    std::string line;

    while (std::getline(iss, line, '\n')) {
      result.push_back(line);
      printf("%s\n", line.c_str());
    }


    return result;
  }();

  auto edsHandle = Upstream::EndpointClusterReroutingManager::get().fetchEdsHandleByCluster(metadata[0]);
  if (edsHandle != nullptr) {
#ifdef TIME_EVAL
    auto leave_now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printf("leaving endpoint handler: %lld\n", leave_now);
#endif
    bool res = edsHandle->replaceHost(metadata[1], std::stoul(metadata[2]), metadata[3], std::stoul(metadata[4]));
    return res
              ? Http::Code::OK : Http::Code::InternalServerError;
  } else {
    return Http::Code::BadRequest;
  }
}

} // namespace Server
} // namespace Envoy