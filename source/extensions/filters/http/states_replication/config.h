//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "source/extensions/filters/http/common/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

using ProtoStatesReplicationPolicy = envoy::extensions::filters::http::states_replication::v3::StatesReplicationPolicy;


class StatesReplicationFilterFactory
    : public Common::FactoryBase<envoy::extensions::filters::http::states_replication::v3::StatesReplication,
        ProtoStatesReplicationPolicy> {
  // it seems we really don't need some specific config
  // just inherits from FactoryBase and register the filter
  // declare a simple .proto in api/envoy/extensions/...
  StatesReplicationFilterFactory() : FactoryBase("envoy.filters.http.states_replication") {}

  Http::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::http::states_replication::v3::StatesReplication &proto_config,
      const std::string &stats_prefix, Server::Configuration::FactoryContext &context) override;

  Router::RouteSpecificFilterConfigConstSharedPtr createRouteSpecificFilterConfigTyped(
      const ProtoStatesReplicationPolicy &policy,
      Server::Configuration::ServerFactoryContext &context,
      ProtobufMessage::ValidationVisitor &validator) override;
};


class StatesReplicationPerRouteConfig : public Router::StatesReplicationPolicy {
public:
  StatesReplicationPerRouteConfig(
      const ProtoStatesReplicationPolicy &config,
      Runtime::Loader &loader) : config_(config), loader_(loader) {}

  bool enabled() const override {
    if (config_.has_enabled()) {
      const auto& filter_enabled = config_.enabled();
      return loader_.snapshot().featureEnabled(filter_enabled.runtime_key(),
                                               filter_enabled.default_value());
    }
    return true;
  }

private:
  const ProtoStatesReplicationPolicy config_;
  Runtime::Loader& loader_;
};

}
}
}
}


