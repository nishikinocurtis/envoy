//
// Created by qiutong on 7/11/23.
//

#pragma once

#include "envoy/storage/storage.h"

#include "source/extensions/filters/http/common/pass_through_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

class StatesReplicationFilter : public Http::PassThroughFilter {
public:
  // record content-length and replication-length at decodeHeader
  // moveOut the buffer and move into Storage
  // need to truncate the body
  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override;


  // call Storage Replicate and continue
private:
  std::shared_ptr<Envoy::States::Storage> storage_manager_;
  std::unique_ptr<Envoy::States::StateObject> state_obj_;
  bool is_attached_ = false;
  uint64_t states_position_;
};

} // namespace States
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
