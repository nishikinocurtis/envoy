//
// Created by curtis on 7/5/23.
//

#include "source/server/fast_reconfig/fast_reconfig_filter.h"

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


}

}
}