//
// Created by curtis on 6/30/23.
//

#pragma once

#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/server/listener_manager.h"
#include "envoy/network/listen_socket.h"

#include "source/common/common/logger.h"

namespace Envoy {
namespace Server {

class FastReconfigServerImpl : public FastReconfigServer,
                               Logger::Loggable<Logger::Id::rr_manager> {

};

} // namespace Server
} // namespace Envoy
