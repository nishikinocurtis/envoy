//
// Created by qiutong on 7/23/23.
//

#include "failover_manager_impl.h"

#include "source/common/storage/storage_impl.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/message_impl.h"
#include "source/common/http/header_map_impl.h"

namespace Envoy {
namespace Failover {

RcdsApiImpl::RcdsApiImpl(const envoy::config::core::v3::ConfigSource &rcds_config, std::shared_ptr<FailoverManager> &&failover_manager,
                         Upstream::ClusterManager &cm, Init::Manager &init_manager, Stats::Scope &scope,
                         ProtobufMessage::ValidationVisitor validation_visitor)
                         : Config::SubscriptionBase<Recovery>(validation_visitor, "name"),
                           failover_manager_(std::move(failover_manager)), scope_(scope.createScope("failover.rcds.")),
                           cm_(cm), init_target_("RCDS", [this]() { subscription_->start({}); }){
  // create subscription

  init_manager.add(init_target_);
}

void
RcdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &resources, const std::string &version_info) {
  std::string recovery_target;
  version_info_ = version_info;

  failover_manager_->registerPreSelectedRecoveryTarget(recovery_target);
}

void
RcdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &added_resources,
                            const Protobuf::RepeatedPtrField<std::string>&, const std::string &system_version_info) {
  // ignore removed resources
  version_info_ = system_version_info;

  std::string recovery_target;
  failover_manager_->registerPreSelectedRecoveryTarget(recovery_target);
}

void
RcdsApiImpl::onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason, const Envoy::EnvoyException *e) {
  ENVOY_LOG(debug, "recovery target updating failed, for reason x");
}

FailoverManagerImpl::FailoverManagerImpl(Event::Dispatcher &dispatcher, const LocalInfo::LocalInfo &local_info,
                                         Upstream::ClusterManager &cm)
                                         : dispatcher_(dispatcher), storage_manager_(States::StorageSingleton::getInstance()),
                                           local_info_(local_info), cm_(cm) {
  // other members to be initialized
}

void FailoverManagerImpl::onLocalFailure() {
  // check pre-selected recovery target
  if (pre_selection_recovery_ != std::nullopt) {
    // if available send recovery signal
    // including resource_ids / pod_id / critical connection in
    auto headers = Http::RequestHeaderMapImpl::create();
    headers->setByKey("x-ftmesh-pod-id", "x");
    // DON'T USE THE LOGIC BELOW!!!
    // compose into Protobuf::Message
    // repeated Predicates or strings (port number) of ongoing services.
    // when handler receives, compose a UDPProxyConfig
    // with matcher, single predicates for IP, ports for inner matcher_map
    // send to critical nodes (by backup node)
    // USE THIS LOGIC INSTEAD:
    // the ongoing services should be included in the metadata already
    // just send the critical connections (pod ids)
    // the other side should compose matcher based on ongoing services and their new sockets.
    // some envoy::config::v3::CriticalPods message;
    // for (const auto& pod : critical_connections_)
    //  message.add_podIds(pod);
    auto body = Buffer::OwnedImpl(); // use Common::serializeMessage
    // auto request makeHttpCall
    // yield 0.
  } else {
    // notify controller
  }

  // don't wait for socket feedback: let the backup nodes broadcast i
}

void FailoverManagerImpl::registerPreSelectedRecoveryTarget(const std::string &target) {
  uint32_t ttl_arg = 10;
  if (pre_selection_recovery_ != std::nullopt) {
    // cancel timer first.
    pre_selection_ttl_->disableTimer();
  }
  pre_selection_recovery_ = target;
  pre_selection_ttl_ = dispatcher_.createTimer([this]() {
        pre_selection_recovery_ = std::nullopt;
      });
  std::chrono::milliseconds ttl(ttl_arg * 1000);
  pre_selection_ttl_->enableTimer(ttl);
}

Http::AsyncClient::Request* FailoverManagerImpl::makeHttpCall(
    const std::string& target, std::unique_ptr<Http::RequestHeaderMap>&& headers,
    Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
    Http::AsyncClient::Callbacks& callbacks) {
  if (headers->Path() == nullptr) {
    // not responsible for filling replicate endpoint
    return nullptr;
  }
  if (headers->Host() == nullptr) {
    // fill in cluster
    headers->setHost(target);
  }
  if (headers->Method() == nullptr) {
    // fill in POST
    headers->setMethod("POST");
  }

  const auto thread_local_cluster = cm_.getThreadLocalCluster(target);
  if (thread_local_cluster == nullptr) {
    ENVOY_LOG(debug, "Cannot find recovery target cluster x");
    return nullptr;
  }

  Http::RequestMessagePtr message(new Http::RequestMessageImpl(std::move(headers)));
  auto body_length = data.length();
  if (body_length != 0) {
    message->body().add(data);
    message->headers().setContentLength(body_length);
  }

  return thread_local_cluster->httpAsyncClient().send(std::move(message), callbacks, options);
}


}
}