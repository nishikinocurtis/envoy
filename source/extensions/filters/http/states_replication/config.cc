//
// Created by qiutong on 7/11/23.
//

#include "source/extensions/filters/http/states_replication/config.h"

#include "source/common/protobuf/utility.h"
#include "envoy/registry/registry.h"

#include "source/extensions/filters/http/states_replication/states_replication_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {


Http::FilterFactoryCb StatesReplicationFilterFactory::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::http::states_replication::v3::StatesReplication& proto_config,
    const std::string& stats_prefix, Server::Configuration::FactoryContext& context) {
  return [](Http::FilterChainFactoryCallbacks& callbacks) {
    callbacks.addStreamFilter(std::make_shared<StatesReplicationFilter>());
  };
}

Router::RouteSpecificFilterConfigConstSharedPtr
StatesReplicationFilterFactory::createRouteSpecificFilterConfigTyped(
    const ProtoStatesReplicationPolicy &policy,
    Server::Configuration::ServerFactoryContext &context, ProtobufMessage::ValidationVisitor&) {
  return std::make_shared<StatesReplicationPerRouteConfig>(policy, context.runtime());
}

// adopted from other filters.
LEGACY_REGISTER_FACTORY(StatesReplicationFilterFactory, Server::Configuration::NamedHttpFilterConfigFactory,
                        "envoy.states_replication");

}
}
}
}
