//
// Created by qiutong on 7/6/23.
//

#pragma once

#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/config/subscription.h"

namespace Envoy {
namespace Server {

using SubscriptionCallbackWeakPtr = std::weak_ptr<Config::SubscriptionCallbacks>;

class ReconfigHandlerBase {
public:
    ReconfigHandlerBase(Server::Instance& server, SubscriptionCallbackWeakPtr&& callbacks)
        : server_(server), callback_(callbacks) {};

protected:
    Server::Instance& server_;
    SubscriptionCallbackWeakPtr callback_;
};

}
}
