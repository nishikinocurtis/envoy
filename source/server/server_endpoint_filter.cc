//
// Created by curtis on 7/5/23.
//

#include "source/server/server_endpoint_filter.h"
#include "source/server/admin/utils.h"

namespace Envoy {
namespace Server {

ServerEndpointFilterBase::ServerEndpointFilterBase(Admin::GenRequestFn server_handler_func)
    : server_handler_fn_(server_handler_func) {}

Http::FilterHeadersStatus ServerEndpointFilterBase::decodeHeaders(Http::RequestHeaderMap& headers,
                                                     bool end_stream) {
  request_headers_ = &headers;
  if (end_stream) {
    onComplete();
  }

  return Http::FilterHeadersStatus::StopIteration;
}

Http::FilterDataStatus ServerEndpointFilterBase::decodeData(Buffer::Instance& data, bool end_stream) {
  // Currently we generically buffer all admin request data in case a handler wants to use it.
  // If we ever support streaming admin requests we may need to revisit this. Note, we must use
  // addDecodedData() here since we might need to perform onComplete() processing if end_stream is
  // true.
  decoder_callbacks_->addDecodedData(data, false);

  if (end_stream) {
    onComplete();
  }

  return Http::FilterDataStatus::StopIterationNoBuffer;
}

Http::FilterTrailersStatus ServerEndpointFilterBase::decodeTrailers(Http::RequestTrailerMap&) {
  onComplete();
  return Http::FilterTrailersStatus::StopIteration;
}

void ServerEndpointFilterBase::onDestroy() {
  for (const auto& callback : on_destroy_callbacks_) {
    callback();
  }
}

void ServerEndpointFilterBase::addOnDestroyCallback(std::function<void()> cb) {
  on_destroy_callbacks_.push_back(std::move(cb));
}

Http::StreamDecoderFilterCallbacks& ServerEndpointFilterBase::getDecoderFilterCallbacks() const {
  // ASSERT(decoder_callbacks_ != nullptr); need loggable
  return *decoder_callbacks_;
}

const Buffer::Instance* ServerEndpointFilterBase::getRequestBody() const {
  return decoder_callbacks_->decodingBuffer();
}

const Http::RequestHeaderMap& ServerEndpointFilterBase::getRequestHeaders() const {
  // ASSERT(request_headers_ != nullptr); need loggable
  return *request_headers_;
}

Http::Utility::QueryParams ServerEndpointFilterBase::queryParams() const {
  absl::string_view path = request_headers_->getPathValue();
  Http::Utility::QueryParams query = Http::Utility::parseAndDecodeQueryString(path);
  if (!query.empty()) {
    return query;
  }

  // Check if the params are in the request's body.
  if (request_headers_->getContentTypeValue() ==
      Http::Headers::get().ContentTypeValues.FormUrlEncoded) {
    const Buffer::Instance* body = getRequestBody();
    if (body != nullptr) {
      query = Http::Utility::parseFromBody(body->toString());
    }
  }

  return query;
}

} // namespace Server
} // namespace Envoy