//
// Created by qiutong on 7/11/23.
//

#include "recover_handler.h"

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
  return Http::Code::OK;
}

}
}