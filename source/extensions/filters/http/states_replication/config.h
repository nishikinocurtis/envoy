//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "source/extensions/filters/http/common/factory_base.h"
#include "envoy/extensions/filters/http/states_replication/v3/states_replication.pb.h"
#include "envoy/extensions/filters/http/states_replication/v3/states_replication.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

using ProtoStatesReplicationPolicy = envoy::extensions::filters::http::states_replication::v3::StatesReplicationPolicy;


class StatesReplicationFilterFactory
    : public Common::FactoryBase<envoy::extensions::filters::http::states_replication::v3::StatesReplication,
        ProtoStatesReplicationPolicy> {
public:
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
    if (config_.enabled()) {
      return loader_.snapshot().featureEnabled("100",
                                               100);
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


