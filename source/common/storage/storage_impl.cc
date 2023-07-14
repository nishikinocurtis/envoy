//
// Created by qiutong on 7/10/23.
//

#include "storage_impl.h"
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

int StorageImpl::makeHttpCall(const Http::AsyncClient::RequestOptions& options,
                 Http::AsyncClient::Callbacks& callbacks) {
  return 0;
}

}
}
