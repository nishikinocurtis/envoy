//
// Created by qiutong on 7/11/23.
//

#include "recover_handler.h"

#include "absl/strings/str_split.h"

#include "envoy/storage/storage.h"

#include "source/common/storage/storage_impl.h"


namespace Envoy {
namespace Server {

/**
 * on receiving the recovery signal and deliver the states to target application
 * @param admin_stream
 * @param response_headers
 * @param response
 * @return
 */
Http::Code ReplicateRecoverHandler::onFailureRecovery(Envoy::Server::AdminStream &admin_stream,
                                                      Http::ResponseHeaderMap &response_headers,
                                                      Buffer::Instance &response) {
  // TODO: TBI
  // call storage_ptr_->recover()
  ENVOY_LOG(debug, "Recovery signal received for service x with resource-id x, method x, "
                   "deliver to port p uri u at time t.");

#ifdef RECOVER_BENCHMARK
  auto time_counter = std::chrono::high_resolution_clock::now();

  auto time_marker = std::string{admin_stream.getRequestHeaders().get(
      Http::LowerCaseString("x-ftmesh-bench-marker")
      )[0]->value().getStringView()};
#endif

  std::vector<std::string> resource_ids = absl::StrSplit(admin_stream.getRequestBody()->toString(), '\n');
  for (const std::string& resource_id : resource_ids) {
    // bool result = false; // this should be returned from recover().
#ifdef RECOVER_BENCHMARK
    storage_ptr_->bench_recover(resource_id, time_marker, time_counter);
#else
    storage_ptr_->recover(resource_id);
    // TODO: parse the response from recover, compose replcaing msg, broadcast to critical connections bound to resource_id.
#endif
  }

  ENVOY_LOG(debug, "preparing response");

  response.add("socket data here");
  // populate headers
  response_headers.setCopy(Http::LowerCaseString("x-ftmesh-recovered"), "true");

  return Http::Code::OK;
}

/**
 * On receiving a (batch of) replication from the original sidecar.
 * @param admin_stream
 * @param response_headers
 * @param response
 * @return
 */
Http::Code ReplicateRecoverHandler::onStatesReplication(Envoy::Server::AdminStream &admin_stream,
                                                        Http::ResponseHeaderMap &response_headers,
                                                        Buffer::Instance &response) {
  // TODO: TBI
  // create StateObject
  // call storage_ptr_->write()
  ENVOY_LOG(debug, "Replication received for resource x, service s, dumping...");
  const auto &headers = admin_stream.getRequestHeaders();
  States::StorageMetadata metadata;
  // parse header into metadata, or consider packed implementation.
  // metadata.resource_id_ = headers.getByKey("x-ftmesh-resource-id").value();
  metadata.resource_id_ =
      std::string{headers.get(Http::LowerCaseString("x-ftmesh-resource-id"))[0]->value().getStringView()};
  auto state_obj = std::make_shared<States::RawBufferStateObject>(metadata);

  // do a const _cast here to avoid copy: change the request body to non-const.
  state_obj->writeObject(const_cast<Buffer::Instance &>(*admin_stream.getRequestBody()));

  storage_ptr_->write(std::move(state_obj), server_.dispatcher());

  response.add("replicated");
  response_headers.setEnvoyUpstreamServiceTime(0ll);

  return Http::Code::OK;
}

}
}