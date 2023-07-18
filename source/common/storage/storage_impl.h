//
// Created by qiutong on 7/10/23.
//

#include "envoy/storage/storage.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/http/async_client.h"

#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"
#include "source/common/init/target_impl.h"
#include "source/common/protobuf/protobuf.h"

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

class Replicator; // temporary

class RpdsApiImpl : public RpdsApi,
                    Envoy::Config::SubscriptionBase<Replicator>,
                    // TODO: should be storage::v3::Replicator
                    Logger::Loggable<Logger::Id::rr_manager> {
public:
  RpdsApiImpl(const envoy::config::core::v3::ConfigSource &rpds_config,
              std::shared_ptr<Storage> &&str_manager,
              Upstream::ClusterManager &cm,
              Init::Manager &init_manager, Stats::Scope &scope,
              ProtobufMessage::ValidationVisitor validation_visitor);

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
                            const EnvoyException* e) override;

  Config::OpaqueResourceDecoderSharedPtr getResourceDecoder() { return resource_decoder_; }

  Config::SubscriptionPtr subscription_;
  std::string version_info_;
  std::shared_ptr<Storage> str_manager_;
  Stats::ScopeSharedPtr scope_;
  Upstream::ClusterManager& cm_;
  Init::TargetImpl init_target_;
};

class StorageImpl : public Storage { // need to be HttpAsyncClientCallbacks
public:

  void write(std::shared_ptr<StateObject>&& obj) override;

  void replicate(const std::string& resource_id) override;
  // need to take in some filter object, which hold a cluster manager.

  // SubscriptionCallback
  // SubscriptionBase<v3::Replication>

private:

  // make it async
  // consider how to make it parallel without blocking.
  // need a cluster manager, but cluster implies auto load-balancing
  // use endpoint
  int makeHttpCall(const std::string& target, Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
                   Http::AsyncClient::Callbacks& callbacks);

  using WeakReferenceStateMap = std::unordered_map<std::string, std::weak_ptr<StateObject>>;

  std::unordered_map<std::string, std::shared_ptr<StateObject>> states_;
  WeakReferenceStateMap by_pod_; // indexed by svc_name+pod_name

  // need a optionGenerator
  // for AsyncClient.send() usage.
  void populateRequestOption();

  // need target cluster names
  // subscription from xDS?
  // create RpDS Api
  // Request: my service name
  // Response: + target cluster names - target cluster names.

  // need a ClusterManager
  // call for cluster_name : cluster_names do clusterManager.find_cluster(cluster_name).asyncClient().send()
  // maintain a quorom counter per version
};

class StorageSingleton {
public:
  StorageSingleton(const StorageSingleton&) = delete;
  StorageSingleton& operator=(const StorageSingleton) = delete;

  static std::shared_ptr<Storage> getOrCreateInstance(
      // with necessary arguments
      ) {
    if (ptr_ == nullptr) {
      ptr_ = new StorageImpl(); //arguments
    } else {
      return ptr_;
    }
  }
private:
  static std::shared_ptr<Storage> ptr_;
  StorageSingleton() = default;
};

} // namespace States
} // namespace Envoy
