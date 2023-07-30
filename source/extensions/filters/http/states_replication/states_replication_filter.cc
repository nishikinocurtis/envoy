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
  std::string resource_id =
      std::string{headers.get(Http::LowerCaseString("x-ftmesh-resource-id"))[0]->value().getStringView()};

  auto content_length =
      std::stoull(std::string{headers.get(Http::LowerCaseString("content-length"))[0]->value().getStringView()}, nullptr);
  // ENVOY_LOG(debug, "content-length string: {}", headers.getByKey("content-length").value_or("0").data());

  ENVOY_LOG(debug, "HTTP Request captured by StatesReplicationFilter");

  if (resource_id.empty() || end_stream) {
    is_attached_ = false;
    return Http::FilterHeadersStatus::Continue;
  }

  ENVOY_LOG(debug, "Entering states replication");

  // populate other fields.
  Envoy::States::StorageMetadata metadata;
  metadata.resource_id_ = resource_id;
  // do bitwise and with uint16mask to avoid overflow.
  metadata.recover_port_ =
      std::stoul(
          std::string{headers.get(Http::LowerCaseString("x-ftmesh-recover-port"))[0]->value().getStringView()}, nullptr)
          & METADATA_FLAG_MASK;
  metadata.flags = std::stoul(
      headers.get(Http::LowerCaseString("x-ftmesh-flags"))[0]->value().getStringView().data(), nullptr)
          & METADATA_FLAG_MASK;
  metadata.ttl_ = std::stoul(
      headers.get(Http::LowerCaseString("x-ftmesh-ttl"))[0]->value().getStringView().data(), nullptr);

  metadata.recover_uri_ = headers.get(Http::LowerCaseString("x-ftmesh-recover-uri"))[0]->value().getStringView();
  metadata.svc_id_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-id"))[0]->value().getStringView();
  metadata.pod_id_ = headers.get(Http::LowerCaseString("x-ftmesh-pod-id"))[0]->value().getStringView();
  metadata.method_name_ = headers.get(Http::LowerCaseString("x-ftmesh-method-name"))[0]->value().getStringView();
  metadata.svc_ip_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-ip"))[0]->value().getStringView();
  metadata.svc_port_ = headers.get(Http::LowerCaseString("x-ftmesh-svc-port"))[0]->value().getStringView();

  states_position_ = std::stoull(
      std::string{headers.get(Http::LowerCaseString("x-ftmesh-states-position"))[0]->value().getStringView()}, nullptr);

  auto new_content_length = states_position_;
  ENVOY_LOG(debug, "mutating headers, old content length: {}, states_position: {}, new content length: {}",
            content_length, states_position_, new_content_length);
  headers.setContentLength(new_content_length);

  ENVOY_LOG(debug, "Metadata extracted, resource-id: {}, states_position: {}",
            metadata.resource_id_, states_position_);
  state_obj_ = std::make_unique<Envoy::States::RawBufferStateObject>(metadata);
  is_attached_ = true;

  // drop the resource-id header
  headers.remove(Http::LowerCaseString("x-ftmesh-resource-id"));
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus StatesReplicationFilter::decodeData(Buffer::Instance &data, bool end_stream) {
  if (!is_attached_) {
    return PassThroughDecoderFilter::decodeData(data, end_stream);
  } else {
    ENVOY_LOG(debug, "start moving buffer");
    state_obj_->getObject().truncateOut(data, states_position_);
    auto callback = decoder_callbacks_;
    Envoy::States::StorageSingleton::getInstance()->write(std::move(state_obj_), callback->dispatcher());
    ENVOY_LOG(debug, "states buffer captured, continue on next filter, clearing stateful counters");
    is_attached_ = false;
    states_position_ = 0;
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