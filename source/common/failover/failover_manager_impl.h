//
// Created by qiutong on 7/23/23.
//

#pragma once

#include "envoy/failover/failover.h"
#include "envoy/storage/storage.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/http/async_client.h"

#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"
#include "source/common/init/target_impl.h"

namespace Envoy {
namespace Failover {

class Recovery;

class RcdsApiImpl : public RcdsApi,
                    Envoy::Config::SubscriptionBase<Recovery>,
                    Logger::Loggable<Logger::Id::rr_manager> {
public:
  RcdsApiImpl(const envoy::config::core::v3::ConfigSource &rcds_config,
              std::shared_ptr<FailoverManager> &&failover_manager,
              Upstream::ClusterManager &cm,
              Init::Manager &init_manager, Stats::Scope &scope,
              ProtobufMessage::ValidationVisitor validation_visitor);

  std::string versionInfo() const override { return version_info_; }

  std::weak_ptr<Config::SubscriptionCallbacks> genSubscriptionCallbackPtr() override {
    return shared_from_this(); // implicitly downgraded, defined at interface level to avoid exposing lifecycle control.
  }

  Config::OpaqueResourceDecoderSharedPtr getResourceDecoderPtr() override {
    return resource_decoder_;
  }

private:
  // Config::SubscriptionCallbacks
  void onConfigUpdate(const std::vector<Config::DecodedResourceRef>& resources,
                      const std::string& version_info) override;
  void onConfigUpdate(const std::vector<Config::DecodedResourceRef>& added_resources,
                      const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                      const std::string& system_version_info) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException* e) override;

  Config::OpaqueResourceDecoderSharedPtr getResourceDecoder() { return resource_decoder_; }

  Config::SubscriptionPtr subscription_;
  std::shared_ptr<FailoverManager> failover_manager_;
  std::string version_info_;
  Stats::ScopeSharedPtr scope_;
  Upstream::ClusterManager& cm_;
  Init::TargetImpl init_target_;
};


class FailoverManagerImpl : public FailoverManager,
                            Logger::Loggable<Logger::Id::rr_manager> {
public:
  FailoverManagerImpl(Event::Dispatcher& dispatcher,
                      // std::shared_ptr<States::Storage>&& storage_manager, just get it from the singleton
                      const LocalInfo::LocalInfo& local_info,
                      Upstream::ClusterManager& cm);

  void notifyCriticalConnections() override;

  void notifyController() override;

  // need parameter: the connection info
  void registerCriticalConnection() override;

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
  const LocalInfo::LocalInfo& local_info_;
  Upstream::ClusterManager& cm_;
  std::optional<std::string> pre_selection_recovery_;
};

} // namespace Failover
} // namespace Envoy
