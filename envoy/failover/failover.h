//
// Created by qiutong on 7/11/23.
//

#include "envoy/common/pure.h"

// may depend on upstream.h

namespace Envoy {
namespace Failover {

class FailoverManager {
public:
  virtual ~FailoverManager() = default;

  virtual void notifyPrioritizedNodes() PURE;
  // push the updates to critical sidecars directly.

  virtual void notifyController() PURE;
  // push the updates to xDS Server, and letting it distribute the new config cluster-widely

  virtual void registerCriticalConnection() PURE;
  // maintain the list of critical nodes/services that has real traffic,
  // or explicitly noted by the administrator.
  // about granularity: per service or per node or per connection?

  virtual void onLocalFailure() PURE;
  // signal backup node (pre-selected periodically), with prioritized nodes list

  virtual void onFailureSignal() PURE;
  // fire Storage.recovery() and block,
  // after necessary socket info gathered, fire notifyP and notifyC.
};

}
}
