//
// Created by qiutong on 7/11/23.
//

#include <string>

#include "source/common/storage/storage_impl.h"
#include "source/extensions/filters/http/states_replication/states_replication_filter.h"


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

#define METADATA_FLAG_MASK ((1u << 16) - 1)

Http::FilterHeadersStatus StatesReplicationFilter::decodeHeaders(Http::RequestHeaderMap &headers, bool end_stream) {
  auto resource_id = headers.getByKey("x-ftmesh-resource-id").value_or("");

  if (resource_id.empty() || end_stream) {
    return Http::FilterHeadersStatus::Continue;
  }

  // populate other fields.
  Envoy::States::StorageMetadata metadata;
  metadata.resource_id_ = resource_id;
  // do bitwise and with uint16mask to avoid overflow.
  metadata.recover_port_ =
      std::stoul(headers.getByKey("x-ftmesh-recover-port").value_or("0").data(), nullptr) & METADATA_FLAG_MASK;
  metadata.flags = std::stoul(headers.getByKey("x-ftmesh-flags").value_or("0").data(), nullptr) & METADATA_FLAG_MASK;
  metadata.ttl_ = std::stoul(headers.getByKey("x-ftmesh-ttl").value_or("0").data(), nullptr);

  metadata.recover_uri_ = headers.getByKey("x-ftmesh-recover-uri").value_or("");
  metadata.svc_id_ = headers.getByKey("x-ftmesh-svc-id").value_or("");
  metadata.pod_id_ = headers.getByKey("x-ftmesh-pod-id").value_or("");
  metadata.method_name_ = headers.getByKey("x-ftmesh-method-name").value_or("");
  metadata.svc_ip_ = headers.getByKey("x-ftmesh-svc-ip").value_or("0.0.0.0");
  metadata.svc_port_ = headers.getByKey("x-ftmesh-svc-port").value_or("0");

  states_position_ = std::stoull(headers.getByKey("x-ftmesh-states-position").value_or("0").data(), nullptr);

  state_obj_ = std::make_unique<Envoy::States::RawBufferStateObject>(metadata);
  is_attached_ = true;
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus StatesReplicationFilter::decodeData(Buffer::Instance &data, bool end_stream) {
  if (!is_attached_) {
    return PassThroughDecoderFilter::decodeData(data, end_stream);
  } else {
    state_obj_->getObject().truncateOut(data, states_position_);
    storage_manager_->write(std::move(state_obj_));
    return Http::FilterDataStatus::Continue;
  }
}

Http::FilterTrailersStatus StatesReplicationFilter::decodeTrailers(Http::RequestTrailerMap &trailers) {
  return PassThroughDecoderFilter::decodeTrailers(trailers);
}

} // namespace States
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy