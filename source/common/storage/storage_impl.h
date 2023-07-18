//
// Created by qiutong on 7/10/23.
//

#include "envoy/storage/storage.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/http/async_client.h"

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
