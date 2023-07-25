//
// Created by qiutong on 7/10/23.
//

#include "storage_impl.h"

#include "envoy/http/codes.h"

#include "source/common/common/assert.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/message_impl.h"
#include "source/common/http/header_map_impl.h"


namespace Envoy {
namespace States {

RawBufferStateObject::RawBufferStateObject(StorageMetadata &metadata) : metadata_(metadata) {
  buf_ = std::make_unique<Buffer::OwnedImpl>();
}

RawBufferStateObject::RawBufferStateObject(Envoy::States::StorageMetadata &metadata, Buffer::Instance &data) :
  RawBufferStateObject(metadata) {
  buf_->move(data);
}

void RawBufferStateObject::writeObject(Buffer::Instance &obj) {
  buf_->move(obj);
}

// no need for RawBufferStateObject to find if the rhs is typed or not
// always rely on the getObject interface to get a buffer.
void RawBufferStateObject::move(Envoy::States::StateObject &rhs) {
//  if (rhs.metadata().flags & TYPED) {
//    // can only move raw buffer.
//    // ENVOY_LOG(debug, "losing typed information")
//    buf_->move(rhs.getObject());
//  } else {
//    buf_->move(rhs.getObject());
//  }
  // refresh cancel timer
  metadata_ = rhs.metadata();
  buf_->move(rhs.getObject());
}

Buffer::Instance &RawBufferStateObject::getObject() {
  return *buf_;
}

RpdsApiImpl::RpdsApiImpl(const std::string &rpds_resource_locator,
                         envoy::config::core::v3::ConfigSource& rpds_config,
                         std::shared_ptr<Storage> &&str_manager,
                         Upstream::ClusterManager &cm,
                         Init::Manager &init_manager, Stats::Scope &scope,
                         ProtobufMessage::ValidationVisitor validation_visitor)
                         : Config::SubscriptionBase<Replicator>(validation_visitor, "name"),
                           str_manager_(std::move(str_manager)), scope_(scope.createScope("storage.rpds.")),
                           cm_(cm), init_target_("RPDS", [this]() { subscription_->start({}); }) {
  // create subscription, must give both resource_locator and rpds_config.
  const auto resource_name = getResourceName();
  subscription_ = cm.subscriptionFactory().collectionSubscriptionFromUrl(
      rpds_resource_locator, rpds_config, resource_name, *scope_, *this, resource_decoder_);

  init_manager.add(init_target_);
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &resources,
                                 const std::string &version_info) {
  version_info_ = version_info;
  //TODO: TBI
  // call the str_manager_->shiftTargetClusters
  auto new_target_list = std::make_unique<std::list<std::string>>();

  str_manager_->beginTargetUpdate();

  for (const auto& target_to_add : resources) {
    std::string target;
    // cast to target
    new_target_list->push_back(target);
  }

  str_manager_->shiftTargetClusters(std::move(new_target_list));

  str_manager_->endTargetUpdate();
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &added_resources,
                                 const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                                 const std::string &system_version_info) {
  version_info_ = system_version_info;

  //TODO: TBI
  str_manager_->beginTargetUpdate();
  // call the str_manager_->removeTargetCluster
  // std::vector<std::string> removed_targets;
  for (const auto& target_to_remove : removed_resources) {
    // removed_targets.push_back(target_to_remove);
    str_manager_->removeTargetCluster(target_to_remove);
  }


  // std::vector<std::string> added_targets;
  for (const auto& target_to_add : added_resources) {
    std::string target;
    // cast from Message
    // added_targets.push_back(target);
    str_manager_->addTargetCluster(target);
  }


  // call the str_manager_->addTargetCluster, remember to drain current traffics.
  str_manager_->endTargetUpdate();
}

void RpdsApiImpl::onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                                       const Envoy::EnvoyException *e) {
  // logic adopted from LdsApiImpl.
  ASSERT(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure != reason);
  ENVOY_LOG(debug, "RpdsAPI config update failed");
  init_target_.ready();
}

StorageImpl::StorageImpl(Event::Dispatcher &dispatcher, Server::Instance& server,
                         const envoy::config::storage::v3::Storage& storage_config, const LocalInfo::LocalInfo &local_info,
                         Upstream::ClusterManager &cm)
                         : dispatcher_(dispatcher), server_(server),
                         rpds_api_(storage_config.rpdsConfig(), shared_from_this(), cm, server_.initManager(),
                                   *server_.stats().rootScope(), server_.messageValidationContext().dynamicValidationVisitor()),
                         local_info_(local_info), cm_(cm) {
  // [SOLVED] need rpds_api_ initialization parameters: get init_manager, scope, and validation visitor from here.
}

void StorageImpl::write(std::shared_ptr<StateObject>&& obj) {
  StorageMetadata metadata = obj->metadata();

  // decide update or create
  // no matter which path, remember to refresh the removal timer.
  if (auto existing = states_.find(metadata.resource_id_); existing != states_.end()) {
    // update instead of insert
    // cancel old timer
    // do some remove event,
    // move new object
    auto timer_old = ttl_timers_.find(metadata.resource_id_);
    timer_old->second->disableTimer();

    existing->second->move(*obj);
  } else {
    auto inserted_pair = states_.insert(std::make_pair(metadata.resource_id_, std::move(obj)));
    // register the iterator to different categories.
    by_pod_.insert(std::make_pair(metadata.pod_id_, inserted_pair.first->second));
  }
  // register new timer.
  // timer = add_event(cb, timeout)
  // cb written here, [this, metadata]() -> { this->states_.remove(metadata.resource_id); }
  auto timer_ptr = dispatcher_.createTimer([this, metadata]() {
    this->states_.erase(metadata.resource_id_);
  });
  std::chrono::milliseconds ttl(metadata.ttl_ * 1000);
  timer_ptr->enableTimer(ttl);

  // register to a unordered_map.
  ttl_timers_[metadata.resource_id_] = std::move(timer_ptr);
}

void StorageImpl::replicate(const std::string &resource_id) {
  // makeHttpCall
  auto it = states_.find(resource_id);

  Http::AsyncClient::RequestOptions options;
  // build options

  if (it != states_.end()) {
    for (const auto& target : *target_clusters_) { // always async
      auto headers = Http::RequestHeaderMapImpl::create();
      // populate, filling in metadata,
      // filling in host and path from configuration
      makeHttpCall(target, std::move(headers),
                   it->second->getObject(), options, *this);
    }
  } else {
    // report not found
    ENVOY_LOG(debug, "resource x not found for replication");
  }

}

// this should not be async, may wait for socket info
void StorageImpl::recover(const std::string& resource_id) {
  // makeHttpCall, combine with recover port and path
  // depending on if the port+path is a handshake port
  auto it = states_.find(resource_id);

  if (it != states_.end()) {
    auto metadata = it->second->metadata();
    Http::AsyncClient::RequestOptions options;

    if (metadata.flags & HANDSHAKE) {
      // handshake logic TBI
      ENVOY_LOG(debug, "non-final port recovery not implemented");
    } else {
      // makeHttpCall, transfer the resource.
      auto headers = Http::RequestHeaderMapImpl::create();
      // populate, fill in host and path from metadata
      headers->setHost(local_info_.address()->logicalName() + std::to_string(metadata.recover_port_));
      // how to be aware of which cluster I am sending to?
      // actually I'm just sending to localhost...
      auto deliver_target = local_info_.clusterName();
      makeHttpCall(deliver_target, std::move(headers), it->second->getObject(), options, *this);
      return;
    }
  } else {
    ENVOY_LOG(debug, "resource x not found for recovery");
  }
}

void StorageImpl::deactivate(const std::string &resource_id) {
  // first cancel the timer
  auto timer_old = ttl_timers_.find(resource_id);
  if (timer_old != ttl_timers_.end()) {
    timer_old->second->disableTimer();
    states_.erase(resource_id);
  } else {
    // report not found
    ENVOY_LOG(debug, "target resource x to be deactivated not found.");
  }
}

Http::AsyncClient::Request* StorageImpl::makeHttpCall(
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
    ENVOY_LOG(debug, "Cannot find replication target cluster x");
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
