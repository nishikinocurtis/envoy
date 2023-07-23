//
// Created by qiutong on 7/23/23.
//

#pragma once

#include "envoy/failover/failover.h"
#include "envoy/storage/storage.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/http/async_client.h"

namespace Envoy {
namespace Failover {

class FailoverManagerImpl : public FailoverManager {
public:
  FailoverManagerImpl(Event::Dispatcher& dispatcher,
                      // std::shared_ptr<States::Storage>&& storage_manager, just get it from the singleton
                      const LocalInfo::LocalInfo& local_info,
                      Upstream::ClusterManager& cm);

  void onLocalFailure() override;
  // signal backup node (pre-selected periodically), with prioritized nodes list

  // void onFailureSignal() override;
  // fire Storage.recovery() and block,
  // after necessary socket info gathered, fire notifyP and notifyC.
private:
  Http::AsyncClient::Request* makeHttpCall(const std::string& target, std::unique_ptr<Http::RequestHeaderMap>&& headers,
                                           Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
                                           Http::AsyncClient::Callbacks& callbacks);

  std::shared_ptr<States::Storage> storage_manager_;
  Upstream::ClusterManager& cm_;
};

} // namespace Failover
} // namespace Envoy
