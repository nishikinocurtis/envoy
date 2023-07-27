//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "envoy/server/instance.h"
#include "envoy/storage/storage.h"
#include "source/common/common/logger.h"

namespace Envoy {
namespace Server {

class ReplicateRecoverHandler : Logger::Loggable<Logger::Id::rr_manager> {
  // read body from AdminStream, extract resource_id,
  // call Storage.recover()
public:
  ReplicateRecoverHandler(Server::Instance& server,
                 std::shared_ptr<States::Storage>&& storage_manager)
                 : server_(server), storage_ptr_(std::move(storage_manager)) {}

  Http::Code onFailureRecovery(AdminStream& admin_stream,
                               Http::ResponseHeaderMap& response_headers,
                               Buffer::Instance& response);
  Http::Code onStatesReplication(AdminStream& admin_stream,
                                 Http::ResponseHeaderMap& response_headers,
                                 Buffer::Instance& response);

private:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-private-field"
  Server::Instance& server_;
#pragma clang diagnostic pop

  std::shared_ptr<States::Storage> storage_ptr_;

};


}
}
