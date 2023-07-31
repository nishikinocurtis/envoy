//
// Created by qiutong on 7/9/23.
//

#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "envoy/network/listener.h"

#include "source/common/common/assert.h"
#include "source/common/common/basic_resource_impl.h"
#include "source/common/http/conn_manager_config.h"
#include "source/common/http/conn_manager_impl.h"
#include "source/common/network/connection_balancer_impl.h"

namespace Envoy {
namespace Server {

class ServerEndpointListener : public Network::ListenerConfig {
public:
  ServerEndpointListener(Stats::ScopeSharedPtr&& listener_scope, const std::string& ctx_name, bool ignore_global_conn_limit)
      : name_(ctx_name), scope_(std::move(listener_scope)),
        stats_(Http::ConnectionManagerImpl::generateListenerStats("http." + ctx_name + ".", *scope_)),
        init_manager_(nullptr), ignore_global_conn_limit_(ignore_global_conn_limit) {}

  // Network::ListenerConfig made partially virtual

  bool bindToPort() const override { return true; }
  bool handOffRestoredDestinationConnections() const override { return false; }
  uint32_t perConnectionBufferLimitBytes() const override { return 0; }
  std::chrono::milliseconds listenerFiltersTimeout() const override { return {}; }
  bool continueOnListenerFiltersTimeout() const override { return false; }
  Stats::Scope& listenerScope() override { return *scope_; }
  virtual uint64_t listenerTag() const override { return 0; }
  const std::string& name() const override { return name_; }
  Network::UdpListenerConfigOptRef udpListenerConfig() override { return {}; }
  Network::InternalListenerConfigOptRef internalListenerConfig() override { return {}; }
  envoy::config::core::v3::TrafficDirection direction() const override {
    return envoy::config::core::v3::UNSPECIFIED;
  }
  Network::ConnectionBalancer& connectionBalancer(const Network::Address::Instance&) override {
    return connection_balancer_;
  }
  ResourceLimit& openConnections() override { return open_connections_; }
  const std::vector<AccessLog::InstanceSharedPtr>& accessLogs() const override {
    return empty_access_logs_;
  }
  uint32_t tcpBacklogSize() const override { return ENVOY_TCP_BACKLOG_SIZE; }
  Init::Manager& initManager() override { return *init_manager_; }
  bool ignoreGlobalConnLimit() const override { return ignore_global_conn_limit_; }

  const std::string name_;
  Stats::ScopeSharedPtr scope_;
  Http::ConnectionManagerListenerStats stats_;
  Network::NopConnectionBalancerImpl connection_balancer_;
  BasicResourceLimitImpl open_connections_;

protected:
  const std::vector<AccessLog::InstanceSharedPtr> empty_access_logs_;
  std::unique_ptr<Init::Manager> init_manager_;
  const bool ignore_global_conn_limit_;
};

}
}