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

  std::vector<std::string> resource_ids = absl::StrSplit(admin_stream.getRequestBody()->toString(), '\n');
  for (const std::string& resource_id : resource_ids) {
    bool result = false; // this should be returned from recover().
    storage_ptr_->recover(resource_id);
  }

  response.add("socket data here");
  // populate headers

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
  auto state_obj = std::make_shared<States::RawBufferStateObject>(metadata);

  // do a const _cast here to avoid copy: change the request body to non-const.
  state_obj->writeObject(const_cast<Buffer::Instance &>(*admin_stream.getRequestBody()));

  storage_ptr_->write(std::move(state_obj));

  response_headers.setEnvoyUpstreamServiceTime(0ll);

  return Http::Code::OK;
}

}
}