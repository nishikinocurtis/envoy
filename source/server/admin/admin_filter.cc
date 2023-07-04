#include "source/server/admin/admin_filter.h"
#include "source/server/admin/utils.h"

namespace Envoy {
namespace Server {

/*
 * Currently copied twice to be compatible with existing implementation.
 */
AdminFilter::AdminFilter(Admin::GenRequestFn admin_handler_fn)
    : ServerEndpointFilterBase(admin_handler_fn),
      admin_handler_fn_(admin_handler_fn) {}

Http::StreamDecoderFilterCallbacks& AdminFilter::getDecoderFilterCallbacks() const {
  ASSERT(decoder_callbacks_ != nullptr);
  return *decoder_callbacks_;
}

const Http::RequestHeaderMap& AdminFilter::getRequestHeaders() const {
  ASSERT(request_headers_ != nullptr);
  return *request_headers_;
}

void AdminFilter::onComplete() {
  absl::string_view path = request_headers_->getPathValue();
  ENVOY_STREAM_LOG(debug, "request complete: path: {}", *decoder_callbacks_, path);

  auto header_map = Http::ResponseHeaderMapImpl::create();
  RELEASE_ASSERT(request_headers_, "");
  Admin::RequestPtr handler = admin_handler_fn_(*this);
  Http::Code code = handler->start(*header_map);
  Utility::populateFallbackResponseHeaders(code, *header_map);
  decoder_callbacks_->encodeHeaders(std::move(header_map), false,
                                    StreamInfo::ResponseCodeDetails::get().AdminFilterResponse);

  bool more_data;
  do {
    Buffer::OwnedImpl response;
    more_data = handler->nextChunk(response);
    bool end_stream = end_stream_on_complete_ && !more_data;
    ENVOY_LOG_MISC(debug, "nextChunk: response.length={} more_data={} end_stream={}",
                   response.length(), more_data, end_stream);
    if (response.length() > 0 || end_stream) {
      decoder_callbacks_->encodeData(response, end_stream);
    }
  } while (more_data);
}

} // namespace Server
} // namespace Envoy
