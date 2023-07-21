//
// Created by qiutong on 7/11/23.
//

#include "source/common/storage/storage_impl.h"
#include "source/extensions/filters/http/states_replication/states_replication_filter.h"


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {


Http::FilterHeadersStatus StatesReplicationFilter::decodeHeaders(Http::RequestHeaderMap &headers, bool end_stream) {
  auto resource_id = headers.getByKey("x-ftmesh-resource-id").value_or("");

  if (resource_id.empty()) {
    return Http::FilterHeadersStatus::Continue;
  }

  // populate other fields.
  Envoy::States::StorageMetadata metadata;
  metadata.resource_id_ = resource_id;

  state_obj_ = std::make_unique<Envoy::States::RawBufferStateObject>(metadata);
  is_attached_ = true;
  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus StatesReplicationFilter::decodeData(Buffer::Instance &data, bool end_stream) {
  if (!is_attached_) {
    return PassThroughDecoderFilter::decodeData(data, end_stream);
  } else {

  }
}

Http::FilterTrailersStatus StatesReplicationFilter::decodeTrailers(Http::RequestTrailerMap &trailers) {
  return PassThroughDecoderFilter::decodeTrailers(trailers);
}

} // namespace States
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy