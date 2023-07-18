//
// Created by qiutong on 7/10/23.
//

#include "storage_impl.h"

#include "source/common/common/assert.h"

#include "source/common/buffer/buffer_impl.h"


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

RpdsApiImpl::RpdsApiImpl(const envoy::config::core::v3::ConfigSource &rpds_config,
                         std::shared_ptr<Storage> &&str_manager,
                         Upstream::ClusterManager &cm,
                         Init::Manager &init_manager, Stats::Scope &scope,
                         ProtobufMessage::ValidationVisitor validation_visitor)
                         : Config::SubscriptionBase<Replicator>(validation_visitor, "name"),
                           str_manager_(std::move(str_manager)), scope_(scope.createScope("storage.rpds.")),
                           cm_(cm), init_target_("RPDS", [this]() { subscription_->start({}); }) {
  // create subscription

  init_manager.add(init_target_);
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &resources,
                                 const std::string &version_info) {
  //TODO: TBI
  // call the str_manager_->shiftTargetClusters
}

void RpdsApiImpl::onConfigUpdate(const std::vector<Config::DecodedResourceRef> &added_resources,
                                 const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                                 const std::string &system_version_info) {
  //TODO: TBI
  // call the str_manager_->removeTargetCluster

  // call the str_manager_->addTargetCluster, remember to drain current traffics.
}

void RpdsApiImpl::onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                                       const Envoy::EnvoyException *e) {
  // logic adopted from LdsApiImpl.
  ASSERT(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure != reason);
  ENVOY_LOG(debug, "RpdsAPI config update failed");
  init_target_.ready();
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
    existing->second->move(*obj);
  } else {
    auto inserted_pair = states_.insert(std::make_pair(metadata.resource_id_, std::move(obj)));
    // register the iterator to different categories.
    by_pod_.insert(std::make_pair(metadata.pod_id_, inserted_pair.first->second));
  }
  // register new timer.
  // timer = add_event(cb, timeout)
  // cb written here, [this, metadata]() -> { this->states_.remove(metadata.resource_id); }
}

void StorageImpl::replicate(const std::string &resource_id) {
  // makeHttpCall
}

int StorageImpl::makeHttpCall(const std::string& target, Buffer::Instance& data, const Http::AsyncClient::RequestOptions& options,
                  Http::AsyncClient::Callbacks& callbacks) {
  return 0;
}

}
}
