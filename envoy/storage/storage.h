//
// Created by qiutong on 7/10/23.
//
#pragma once

#include <string>
#include <vector>
#include <optional>

#include "envoy/config/core/v3/config_source.pb.h"
#include "envoy/event/dispatcher.h"

#include "envoy/config/subscription.h"
#include "envoy/common/pure.h"
#include "envoy/buffer/buffer.h"

#pragma once

namespace Envoy {
namespace States {

enum StorageMetaDataFlags : uint16_t {
  TYPED = 1,
  HANDSHAKE = 1 << 1

};

class StorageMetadata {
  // service, pod, module, session, version
  // ttl
  // recover handshake port, uri.
public:
  uint16_t recover_port_, flags;
  uint32_t ttl_;
  std::string recover_uri_, resource_id_, svc_id_, pod_id_, method_name_;
  std::string svc_ip_, svc_port_;
  // ip generated from local_info_ on first receiving the states, port attached by application.
  // need design choice: how to implement incremental (delta) storage?
};

class StateObject {
public:
  virtual ~StateObject() = default;
  virtual const StorageMetadata& metadata() const {
    return metadata_;
  }

  virtual void writeObject(Buffer::Instance& obj) PURE;

  virtual Buffer::Instance& getObject() PURE;

  virtual void move(StateObject& rhs) PURE;

protected:
  StorageMetadata metadata_;
};

class RecoverRoutingInfo {
public:
  uint32_t _port;
  std::string _ip;
};

class RpdsApi { // RePlication Discovery Service
public:
  virtual ~RpdsApi() = default;

  /**
   * @return std::string latest version received from RpDS
   */
   virtual std::string versionInfo() const PURE;

  /**
  * @return std::weak_ptr<const Config::SubscriptionCallbacks> for others to pierce into Lds Management.
  */
  virtual std::weak_ptr<Config::SubscriptionCallbacks> genSubscriptionCallbackPtr() {
    return std::weak_ptr<Config::SubscriptionCallbacks>(); // if it doesn't support, return empty ptr.
  };

  /**
   * @return Config::OpaqueResourceDecoderSharedPtr for external updating usage.
   */
  virtual Config::OpaqueResourceDecoderSharedPtr getResourceDecoderPtr() {
    return nullptr; // if it doesn't support, return nullptr.
  }
};

using RpdsApiPtr = std::shared_ptr<RpdsApi>;

/**
 * Low level distributed storage management interface, to be used by state replication module.
 */

class Storage : public std::enable_shared_from_this<Storage> {
public:
  virtual ~Storage() = default;

  virtual int32_t validate_target(const std::string_view& host) const PURE;

  /**
   * create or update a StateObject.
   * Fired by StateReplicationFilter on outgoing request,
   * or by ReplicateHandler on incoming endpoint request,
   * or by StateReplicationFilter on incoming request? (TBD by config).
   * @param obj the Object to write, uniquely identified by a resource_id, can be referenced
   * through service_id, pod_id, etc. need to register the obj in different map, register
   * cleanup event with ttl (or default ttl) timeout.
   */
  virtual void write(std::shared_ptr<StateObject>&& obj, Event::Dispatcher& tls_dispatcher //, some bytes array, or object
  ) PURE;

  /*
   * Write raw bytes (plus metadata, consider fixing the header size) to the lsm ring buf data structure
   * Return status code: if exceed the siz limit: trigger replicate() itself, and return 1
   * Otherwise return nullopt
   */
  virtual std::unique_ptr<Buffer::Instance> write_lsm_attach(std::shared_ptr<StateObject>&& obj,
                                                             Event::Dispatcher& tls_dispatcher,
                                                             int32_t target) PURE;

  virtual int32_t write_lsm_force(std::shared_ptr<StateObject>&& obj, Event::Dispatcher& tls_dispatcher) PURE;

  /*
   * Simulate the result of write_lsm.
   */
  virtual int32_t validate_write_lsm(int32_t siz, int32_t target) const PURE;

  /**
   * replicate the StateObject identified by the resource_id to other sidecars.
   * The range is specified other ways.
   * @param resource_id locator for the stateObject to be replicated.
   */
  virtual void replicate(const std::string& resource_id) PURE;

  // Should support packed transmission.
  virtual void replicate(std::vector<std::string>& resource_ids) PURE;

  // TODO: consider if we can pack up this.
  virtual void replicateSvc(const std::string& service_id) PURE;

  /**
   * Deliver the stateObject in raw bytes manner to pre-described recover port and uri.
   * Fired by FailoverManager.onFailureSignal().
   * @param resource_id locator for the stateObject to be recovered.
   * @param target the backup node to contact and deliver SObj to.
   * @return should return some socket info from the application as response.
   */
   // why target is specified only when recovering?
   // It is maintained by FailOver manager periodically so changes more frequently
   // than a real resource, and is effective for all resources under some certain svc.pod.
   // So iterate and modify the backup target every time is less efficient
   // Why don't do this for ttl? recovery port / uri?
   // Recovery port/uri can be negotiated more ahead of time.
  virtual void recover(const std::string& resource_id) PURE;

  virtual void bench_recover(const std::string& resource_id,
                             const std::string& bench_marker,
                             std::chrono::time_point<std::chrono::high_resolution_clock> clock) PURE;

  // Not supporting packed transmission, just for calling convenience.
  virtual void recoverPacked(std::vector<std::string>& resource_ids) PURE;

  // TODO: consider if we can pack up this.
  virtual void recoverSvc(const std::string& service_id) PURE;

  /**
   * on session destroy, graceful shutdown, clean up the state and release the memory
   * maybe broadcast to other nodes also.
   * @param resource_id locator for the StateObject to be removed.
   */
  virtual void deactivate(const std::string& resource_id) PURE;

  virtual void deactivate(std::vector<std::string>& resource_ids) PURE;

  virtual void deactivateSvc(const std::string& service_id) PURE;

  virtual void beginTargetUpdate() PURE;

  virtual void endTargetUpdate() PURE; // failure info

  // receive DeltaRpdsTargetUpdate from xDS requires a decent design of how we locate a replication target.
  // the change could happen in two level (or we cheat here to remove the svc level): which svc we recognize and
  // which pods to replicate to upon receiving states from this svc.
  // If we force that one envoy instance can serve as sidecar for only one service at any time, then it becomes easy:
  // just a List of string for memory efficiency
  // on service shift: just discarding the whole list and initialize a new one.
  // on normal update: insert or remove from the list. (O(n), this wouldn't incur much overhead, since it's much less
  // frequently, and the replication process itself consumes O(n) time).
  virtual void addTargetCluster(const std::string& cluster) PURE;

  virtual void shiftTargetClusters(std::unique_ptr<std::list<std::string>>&& sync_target,
                                   std::unique_ptr<std::set<std::string>>&& priority_ups,
                                   std::unique_ptr<std::list<std::string>>&& sync_target_host) PURE;

  virtual void removeTargetCluster(const std::string& cluster) PURE;


protected:
  virtual void timedCleanUp() PURE;
};

} // namespace States
} // namespace Envoy


