//
// Created by qiutong on 7/10/23.
//
#pragma once

#include "envoy/storage/storage.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/http/async_client.h"
#include "envoy/event/timer.h"
#include "envoy/server/instance.h"

#include "envoy/service/discovery/v3/discovery.pb.h"
#include "envoy/config/storage/v3/storage.pb.h"
#include "envoy/config/storage/v3/storage.pb.validate.h"
#include "envoy/config/storage/v3/replicator.pb.h"
#include "envoy/config/storage/v3/replicator.pb.validate.h"

#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"
#include "source/common/init/target_impl.h"
#include "source/common/protobuf/protobuf.h"
#include "source/common/config/xds_resource.h"

namespace Envoy {
namespace States {

class RawBufferStateObject : public StateObject {
public:
  explicit RawBufferStateObject(StorageMetadata& metadata);
  RawBufferStateObject(StorageMetadata& metadata, Buffer::Instance& data);

  void writeObject(Buffer::Instance& obj) override;
  Buffer::Instance& getObject() override;
  void move(StateObject& rhs) override;

private:
  std::unique_ptr<Buffer::Instance> buf_;
};

using Replicator = envoy::config::storage::v3::Replicator;

class RpdsApiImpl : public RpdsApi,
                    Envoy::Config::SubscriptionBase<Replicator>,
                    Logger::Loggable<Logger::Id::rr_manager> {
public:
  RpdsApiImpl(const xds::core::v3::ResourceLocator* rpds_resource_locator,
              const envoy::config::core::v3::ConfigSource& rpds_config,
              std::shared_ptr<Storage> &&str_manager,
              Upstream::ClusterManager &cm,
              Init::Manager &init_manager, Stats::Scope &scope,
              ProtobufMessage::ValidationVisitor& validation_visitor);

  // States::RpdsApi
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
                            const EnvoyException*) override;

  Config::OpaqueResourceDecoderSharedPtr getResourceDecoder() { return resource_decoder_; }

  Config::SubscriptionPtr subscription_;
  std::string version_info_;
  std::shared_ptr<Storage> str_manager_;
  Stats::ScopeSharedPtr scope_;
  Upstream::ClusterManager& cm_;
  Init::TargetImpl init_target_;
};

class StorageImpl : public Storage,
                    public Http::AsyncClient::Callbacks,
                    Logger::Loggable<Logger::Id::rr_manager> { // need to be HttpAsyncClientCallbacks
public:
  StorageImpl(Event::Dispatcher& dispatcher,
              Server::Instance& server,
              const xds::core::v3::ResourceLocator* rpds_resource_locator,
              const envoy::config::storage::v3::Storage& storage_config,
              const LocalInfo::LocalInfo& local_info,
              Upstream::ClusterManager& cm,
              uint32_t lsm_ring_buf_siz);

  void write(std::shared_ptr<StateObject>&& obj, Event::Dispatcher& tls_dispatcher) override;

  // call makeHttpCall to issue request, pass the target cluster name to it.
  void replicate(const std::string& resource_id) override;

  // Should support packed transmission
  void replicate(std::vector<std::string>&) override {}

  // Packed transmission in another dimension
  void replicateSvc(const std::string&) override {}

  void recover(const std::string& resource_id) override;

  void bench_recover(const std::string& resource_id,
                     const std::string& bench_marker,
                     std::chrono::time_point<std::chrono::high_resolution_clock> clock) override;

  // Consider supporting packed transmission.
  void recoverPacked(std::vector<std::string>&) override {}

  // TODO: consider if we can pack up this.
  void recoverSvc(const std::string&) override {}

  void deactivate(const std::string& resource_id) override;

  void deactivate(std::vector<std::string>&) override {}

  void deactivateSvc(const std::string&) override {}

  void addTargetCluster(const std::string& cluster) override {
    target_clusters_->push_back(cluster);
  }
  void shiftTargetClusters(std::unique_ptr<std::list<std::string>>&& sync_target,
                           std::unique_ptr<std::set<std::string>>&& priority_ups) override {
    target_clusters_ = std::move(sync_target);
    priority_upstreams_ = std::move(priority_ups);
  }
  void removeTargetCluster(const std::string& cluster) override {
    for (auto iter = target_clusters_->begin(); iter != target_clusters_->end(); ++iter) {
      if (!iter->compare(cluster)) {
        target_clusters_->erase(iter);
        break;
      }
    }
  }

  void addPriorityUpstream(const std::string& cluster) {
    priority_upstreams_->insert(cluster);
  }
  void shiftPriorityUpstreams(std::unique_ptr<std::set<std::string>>&& cluster_list) {
    priority_upstreams_ = std::move(cluster_list);
  }

  void beginTargetUpdate() override {}
  void endTargetUpdate() override {}

  // deprecated, now registered as lambda when write().
  void timedCleanUp() override {}
  // Http::AsyncClient::Callbacks
  void onSuccess(const Http::AsyncClient::Request&, Http::ResponseMessagePtr&& response) override {
    auto stop = std::chrono::high_resolution_clock::now();
    auto bench_marker = response->headers().get(Http::LowerCaseString("x-ftmesh-bench-marker"));
    if (bench_marker.size() != 0) {

      auto start = clock_keeper_[std::string{bench_marker[0]->value().getStringView()}];
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
      ENVOY_LOG(info, "benchmarking deliver time: {}ms", static_cast<double>(duration.count() / 1000.0));
    }
  }
  void onFailure(const Http::AsyncClient::Request&, Http::AsyncClient::FailureReason) override {}
  void onBeforeFinalizeUpstreamSpan(Tracing::Span&, const Http::ResponseHeaderMap*) override {}

private:
  class RecoverInfoCallback : public Http::AsyncClient::Callbacks {
  public:
    RecoverInfoCallback(StorageImpl& parent) : parent_(parent) {};
    void onSuccess(const Http::AsyncClient::Request&, Http::ResponseMessagePtr&& response) override;
    void onFailure(const Http::AsyncClient::Request&, Http::AsyncClient::FailureReason) override {}
    void onBeforeFinalizeUpstreamSpan(Tracing::Span&, const Http::ResponseHeaderMap*) override {}
  private:
    StorageImpl& parent_;
  };
  using MicroClock = std::chrono::time_point<std::chrono::high_resolution_clock>;

  // make it async
  // consider how to make it parallel without blocking.
  // need a cluster manager, but cluster implies auto load-balancing
  // use endpoint
  Http::AsyncClient::Request* makeHttpCall(const std::string& target, std::unique_ptr<Http::RequestHeaderMap>&& headers,
                                           Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
                                           Http::AsyncClient::Callbacks& callbacks);

  using WeakReferenceStateMap = std::unordered_map<std::string, std::weak_ptr<StateObject>>;

  std::unordered_map<std::string, Event::TimerPtr> ttl_timers_;
  std::unordered_map<std::string, std::shared_ptr<StateObject>> states_;
  std::unordered_map<std::string, uint32_t> ttl_counter_;
  std::unordered_map<std::string, MicroClock> clock_keeper_;
  WeakReferenceStateMap by_pod_; // indexed by svc_name+pod_nam

  std::unique_ptr<std::vector<StateObject>> lsm_pool_; // construct a ring_buf like lsm.
  // change it to buffer, with maximum size, when exceeding do flush
  // set up a timer, when exceeding do flush
  // otherwise, flush the buffer at every filter execution when targeting sync_target_.

  // need a optionGenerator
  // for AsyncClient.send() usage.
  void populateRequestOption();

  Event::Dispatcher& dispatcher_;
  Server::Instance& server_;

  // need target cluster names
  // subscription from xDS?
  // create RpDS Api
  // Request: my service name
  // Response: + target cluster names - target cluster names.
  RpdsApiPtr rpds_api_;

  std::unique_ptr<std::list<std::string>> target_clusters_;
  std::unique_ptr<std::set<std::string>> priority_upstreams_;
  std::unique_ptr<std::list<std::string>> static_upstreams_;

  // need a ClusterManager
  // call for cluster_name : cluster_names do clusterManager.find_cluster(cluster_name).asyncClient().send()
  // maintain a quorom counter per version
  const LocalInfo::LocalInfo& local_info_;
  Upstream::ClusterManager& cm_;

  // callback handler for recover
  std::unique_ptr<RecoverInfoCallback> recover_info_callback_;
};

class StorageSingleton {
public:
  StorageSingleton(const StorageSingleton&) = delete;
  StorageSingleton& operator=(const StorageSingleton) = delete;

  static std::shared_ptr<Storage> getOrCreateInstance(
      Event::Dispatcher& dispatcher,
      Server::Instance& server,
      const std::string& rpds_resource_locator,
      const envoy::config::storage::v3::Storage& storage_config,
      const LocalInfo::LocalInfo& local_info,
      Upstream::ClusterManager& cm
      // with necessary arguments
      // call this from server.cc to initialize.
      ) {
    if (ptr_ == nullptr) {
      std::unique_ptr<xds::core::v3::ResourceLocator> rpds_resource_locator_ptr;
      if (!rpds_resource_locator.empty()) {
        rpds_resource_locator_ptr =
            std::make_unique<xds::core::v3::ResourceLocator>(Config::XdsResourceIdentifier::decodeUrl(
                rpds_resource_locator));
      }
      ptr_ = std::make_shared<StorageImpl>(dispatcher, server,
                                           rpds_resource_locator_ptr.get(), storage_config, local_info, cm); //arguments
      return ptr_;
    } else {
      return ptr_;
    }
  }

  static std::shared_ptr<Storage> getInstance() {
    if (ptr_ == nullptr) {
      // should log invalid here
      return nullptr;
    }
    return ptr_;
  }
private:
  static std::shared_ptr<Storage> ptr_;
  StorageSingleton() = default;
};

} // namespace States
} // namespace Envoy
