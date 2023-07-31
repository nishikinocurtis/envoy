//
// Created by curtis on 7/5/23.
//

#include "source/server/fast_reconfig/fast_reconfig_filter.h"
#include "source/server/utils.h"

namespace Envoy {
namespace Server {

FastReconfigFilter::FastReconfigFilter(GenRequestFn fast_reconfig_handler_func)
    : server_handler_fn_(fast_reconfig_handler_func) {}

Http::StreamDecoderFilterCallbacks& FastReconfigFilter::getDecoderFilterCallbacks() const {
  ASSERT(decoder_callbacks_ != nullptr);
  return *decoder_callbacks_;
}

const Http::RequestHeaderMap& FastReconfigFilter::getRequestHeaders() const {
  ASSERT(request_headers_ != nullptr);
  return *request_headers_;
}

void FastReconfigFilter::onComplete() {
  absl::string_view path = request_headers_->getPathValue();
  ENVOY_STREAM_LOG(debug, "request complete: path: {}", *decoder_callbacks_, path);

  auto handler = server_handler_fn_(*this);
  auto responseStream = handler->process();

  Utility::populateFallbackResponseHeaders(responseStream->getMessageStatus(),
                                           responseStream->getMessageHeader());
  decoder_callbacks_->encodeHeaders(
      responseStream->moveMessageHeader(), false,
                                    StreamInfo::ResponseCodeDetails::get().FastReconfigFilterResponse);

  bool more_data;
  std::unique_ptr<Buffer::Instance> chunk;
  do {
    std::tie(more_data, chunk) = responseStream->fetchNextMessageBodyChunk();
    bool end_stream = end_stream_on_complete_ && !more_data;
    ENVOY_LOG_MISC(debug, "nextChunk: response.length={} more_data={} end_stream={}",
                   chunk->length(), more_data, end_stream);
    if (chunk->length() > 0 || end_stream) {
      decoder_callbacks_->encodeData(*chunk, end_stream);
    }
  } while (more_data);
}

}
}