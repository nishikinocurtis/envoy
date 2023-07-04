//
// Created by curtis on 7/5/23.
//

#include <functional>
#include <list>

#include "envoy/http/filter.h"
#include "envoy/server/admin.h"
#include "envoy/common/pure.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
#include "source/common/http/codes.h"
#include "source/common/http/header_map_impl.h"
#include "source/extensions/filters/http/common/pass_through_filter.h"

#include "absl/strings/string_view.h"

namespace Envoy {
namespace Server {

class ServerEndpointFilterBase : public Http::PassThroughFilter,
                                 public AdminStream {
public:
  ServerEndpointFilterBase() = default;
  // Http::StreamFilterBase
  // Handlers relying on the reference should use addOnDestroyCallback()
  // to add a callback that will notify them when the reference is no
  // longer valid.
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
  Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override;

  // AdminStream
  void setEndStreamOnComplete(bool end_stream) override { end_stream_on_complete_ = end_stream; }
  void addOnDestroyCallback(std::function<void()> cb) override;
  Http::StreamDecoderFilterCallbacks& getDecoderFilterCallbacks() const override;
  const Buffer::Instance* getRequestBody() const override;
  const Http::RequestHeaderMap& getRequestHeaders() const override;
  Http::Http1StreamEncoderOptionsOptRef http1StreamEncoderOptions() override {
    return encoder_callbacks_->http1StreamEncoderOptions();
  }
  Http::Utility::QueryParams queryParams() const override;


protected:
  /**
   * Called when an admin request has been completely received.
   */
  virtual void onComplete() PURE;
  bool end_stream_on_complete_ = true;
  Http::RequestHeaderMap* request_headers_{};
  std::list<std::function<void()>> on_destroy_callbacks_;
};

}
}
