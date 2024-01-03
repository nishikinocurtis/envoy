//
// Created by qiutong on 7/11/23.
//

#pragma once

#include <chrono>

#include "envoy/storage/storage.h"

#include "source/common/common/logger.h"
#include "source/extensions/filters/http/common/pass_through_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace States {

class StatesReplicationFilter : public Http::PassThroughFilter,
                                Logger::Loggable<Logger::Id::rr_manager> {
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
  Http::RequestHeaderMap& headers_;
  std::unique_ptr<Envoy::States::StateObject> state_obj_;
  bool is_attached_ = false;
  uint64_t states_position_;
  uint32_t state_mode_;
  int32_t buf_status_, target_no_;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_counter_;
};

} // namespace States
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
