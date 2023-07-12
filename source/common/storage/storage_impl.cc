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

}
}
