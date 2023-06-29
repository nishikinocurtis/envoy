//
// Created by curtis on 6/30/23.
//

#pragma once

#include <functional>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/pure.h"
#include "envoy/grpc/context.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Server {

/**
 * A listening grpc server abstraction for fast update of listener configuration,
 * which bypasses the normal update->control plane->notification path.
 * However it aims at being compatible with xDS subscriptions, by mutating
 * the hash value and version_info.
 */

class FastReconfigServer {

};


} // namespace Server
} // namespace Envoy

