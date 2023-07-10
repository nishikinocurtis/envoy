//
// Created by qiutong on 7/10/23.
//

#include <string>

#include "envoy/common/pure.h"
#include "envoy/buffer/buffer.h"

#pragma once

namespace Envoy {
namespace States {

class StorageMetadata {
  // service, pod, module, session, version
  // ttl
  // recover handshake port, uri.

  // need design choice: how to implement incremental (delta) storage?
};

/**
 * Low level distributed storage management interface, to be used by state replication module.
 */

class Storage {
public:
  virtual void write(StorageMetadata& metadata, Buffer::Instance& obj //, some bytes array, or object
  ) PURE;

  virtual void replicate(const std::string& resource_id) PURE;

  // Not supporting packed transmission, just for calling convenience.
  virtual void replicate(std::vector<const std::string &> resource_ids) PURE;

  // TODO: consider if we can pack up this.
  virtual void replicateSvc(const std::string& service_id) PURE;

  virtual void recover(const std::string& resource_id) PURE;

  // Not supporting packed transmission, just for calling convenience.
  virtual void recover(std::vector<const std::string &> resource_ids) PURE;

  // TODO: consider if we can pack up this.
  virtual void recoverSvc(const std::string& service_id) PURE;

  virtual void deactivate(const std::string& resource_id) PURE;

  virtual void deactivate(std::vector<const std::string &> resource_ids) PURE;

  virtual void deactivateSvc(const std::string& service_id) PURE;
};

} // namespace States
} // namespace Envoy


