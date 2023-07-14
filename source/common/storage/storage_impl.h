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

class StorageImpl : public Storage {
public:

  void write(std::shared_ptr<StateObject>&& obj) override;

  void replicate(const std::string& resource_id) override;
  // need to take in some filter object, which hold a cluster manager.

private:

  // consider how to make it parallel without blocking.
  int makeHttpCall(const Http::AsyncClient::RequestOptions& options,
                   Http::AsyncClient::Callbacks& callbacks);

  using WeakReferenceStateMap = std::unordered_map<std::string, std::weak_ptr<StateObject>>;

  std::unordered_map<std::string, std::shared_ptr<StateObject>> states_;
  WeakReferenceStateMap by_pod_; // indexed by svc_name+pod_name
  //
};

} // namespace States
} // namespace Envoy
