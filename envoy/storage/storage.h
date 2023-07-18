//
// Created by qiutong on 7/10/23.
//
#pragma once

#include <string>
#include <vector>

#include "envoy/config/core/v3/config_source.pb.h"

#include "envoy/config/subscription.h"
#include "envoy/common/pure.h"
#include "envoy/buffer/buffer.h"

#pragma once

namespace Envoy {
namespace States {

enum StorageMetaDataFlags : uint16_t {
  TYPED = 1
};

class StorageMetadata {
  // service, pod, module, session, version
  // ttl
  // recover handshake port, uri.
public:
  uint16_t recover_port_, flags;
  uint32_t ttl_;
  std::string recover_uri_, resource_id_, svc_id_, pod_id_, method_name_;
  // need design choice: how to implement incremental (delta) storage?
};

class StateObject {
public:
  virtual const StorageMetadata& metadata() const {
    return metadata_;
  }

  virtual void writeObject(Buffer::Instance& obj) PURE;

  virtual Buffer::Instance& getObject() PURE;

  virtual void move(StateObject& rhs) PURE;

protected:
  StorageMetadata metadata_;
};

class RpdsApi {
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

class Storage {
public:
  /**
   * create or update a StateObject.
   * Fired by StateReplicationFilter on outgoing request,
   * or by ReplicateHandler on incoming endpoint request,
   * or by StateReplicationFilter on incoming request? (TBD by config).
   * @param obj the Object to write, uniquely identified by a resource_id, can be referenced
   * through service_id, pod_id, etc. need to register the obj in different map, register
   * cleanup event with ttl (or default ttl) timeout.
   */
  virtual void write(std::shared_ptr<StateObject>&& obj //, some bytes array, or object
  ) PURE;

  /**
   * replicate the StateObject identified by the resource_id to other sidecars.
   * The range is specified other ways.
   * @param resource_id locator for the stateObject to be replicated.
   */
  virtual void replicate(const std::string& resource_id) PURE;

  // Not supporting packed transmission, just for calling convenience.
  virtual void replicate(std::vector<const std::string &> resource_ids) PURE;

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

  // Not supporting packed transmission, just for calling convenience.
  virtual void recover(std::vector<const std::string &> resource_ids) PURE;

  // TODO: consider if we can pack up this.
  virtual void recoverSvc(const std::string& service_id) PURE;

  /**
   * on session destroy, graceful shutdown, clean up the state and release the memory
   * maybe broadcast to other nodes also.
   * @param resource_id locator for the StateObject to be removed.
   */
  virtual void deactivate(const std::string& resource_id) PURE;

  virtual void deactivate(std::vector<const std::string &> resource_ids) PURE;

  virtual void deactivateSvc(const std::string& service_id) PURE;

  virtual void createRpdsApi(const envoy::config::core::v3::ConfigSource& rpds_config) PURE;

  virtual void beginTargetUpdate() PURE;

  virtual void endTargetUpdate() PURE; // failure info

  // receive DeltaRpdsTargetUpdate from xDS requires a decent design of how we locate a replication target.
  virtual void

protected:
  virtual void timedCleanUp() PURE;
};

} // namespace States
} // namespace Envoy


