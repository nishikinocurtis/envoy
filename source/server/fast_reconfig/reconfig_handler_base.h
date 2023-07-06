//
// Created by qiutong on 7/6/23.
//

#pragma once

#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/config/subscription.h"

namespace Envoy {
namespace Server {

class ReconfigHandlerBase {
public:
    ReconfigHandlerBase(Server::Instance& server, Config::SubscriptionCallbacks& callbacks)
        : server_(server), callback_(callbacks) {};

protected:
    Server::Instance& server_;
    Config::SubscriptionCallbacks& callback_;
};

}
}
