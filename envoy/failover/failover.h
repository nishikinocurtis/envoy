//
// Created by qiutong on 7/11/23.
//

#pragma once

#include <string>
#include <vector>

#include "envoy/config/core/v3/config_source.pb.h"

#include "envoy/common/pure.h"
#include "envoy/config/subscription.h"

// may depend on upstream.h

namespace Envoy {
namespace Failover {


class RcdsApi { // ReCovery Discovery Service
public:
  virtual ~RcdsApi() = default;

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

using RcdsApiPtr = std::shared_ptr<RcdsApi>;

class FailoverManager {
public:
  virtual ~FailoverManager() = default;

  virtual void notifyController() PURE;
  // push the updates to xDS Server, and letting it distribute the new config cluster-widely

  virtual void registerCriticalConnection(const std::string& downstream) PURE;
  // maintain the list of critical nodes/services that has real traffic,
  // or explicitly noted by the administrator.
  // about granularity: per service or per node or per connection?

  virtual void onLocalFailure() PURE;
  // signal backup node (pre-selected periodically), with prioritized nodes list

  // deprecated: let filter call Storage.recovery directly.
  // virtual void onFailureSignal() PURE;
  // fire Storage.recovery() and block,
  // after necessary socket info gathered, fire notifyP and notifyC.

  virtual void registerPreSelectedRecoveryTarget(const std::string& target) PURE;


};

}
}
