#pragma once

#include <functional>
#include <list>

#include "envoy/http/filter.h"
#include "envoy/server/admin.h"

#include "source/server/server_endpoint_filter.h"


#include "absl/strings/string_view.h"

namespace Envoy {
namespace Server {

/**
 * A terminal HTTP filter that implements server admin functionality.
 */
class AdminFilter : public ServerEndpointFilterBase,
                    Logger::Loggable<Logger::Id::admin> {
public:
  using AdminServerCallbackFunction = std::function<Http::Code(
      absl::string_view path_and_query, Http::ResponseHeaderMap& response_headers,
      Buffer::OwnedImpl& response, AdminFilter& filter)>;

  AdminFilter(Admin::GenRequestFn admin_handler_func);

  // AdminStream, override for logging.
  Http::StreamDecoderFilterCallbacks& getDecoderFilterCallbacks() const override;
  const Http::RequestHeaderMap& getRequestHeaders() const override;

private:
  /**
   * Called when an admin request has been completely received.
   */
  void onComplete() override;
  Admin::GenRequestFn admin_handler_fn_;
};

} // namespace Server
} // namespace Envoy
