//
// Created by curtis on 6/30/23.
//

#pragma once

#include "envoy/network/filter.h"
#include "envoy/network/listen_socket.h"
#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/server/listener_manager.h"
#include "envoy/storage/storage.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/empty_string.h"
#include "source/common/http/conn_manager_config.h"
#include "source/common/http/conn_manager_impl.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/http/date_provider_impl.h"
#include "source/common/http/default_server_string.h"
#include "source/common/http/http1/codec_stats.h"
#include "source/common/http/http2/codec_stats.h"
#include "source/common/stats/isolated_store_impl.h"
#include "source/server/fast_reconfig/fast_reconfig_filter.h"
#include "source/server/fast_reconfig/listener_handler.h"
#include "source/server/fast_reconfig/recover_handler.h"
#include "source/server/null_overload_manager.h"
#include "source/server/server_endpoint_listener.h"
#include "source/server/server_endpoint_filter.h"
#include "source/server/server_endpoint_socket_factory.h"

#include "source/extensions/listener_managers/listener_manager/lds_api.h"

#include "source/common/common/logger.h"

namespace Envoy {
namespace Server {

class FastReconfigServerInternalAddressConfig : public Http::InternalAddressConfig {
  bool isInternalAddress(const Network::Address::Instance&) const override { return false; }
};

class FastReconfigServerImpl : public FastReconfigServer,
                               public Network::FilterChainManager,
                               public Network::FilterChainFactory,
                               public Http::FilterChainFactory,
                               public Http::ConnectionManagerConfig,
                               Logger::Loggable<Logger::Id::rr_manager> {
public:
  FastReconfigServerImpl(Server::Instance& server,
                         bool ignore_global_conn_limit,
                         SubscriptionCallbackWeakPtr&& listener_reconfig_callback,
                         Config::OpaqueResourceDecoderSharedPtr&& listener_reconfig_resource_decoder,
                         std::shared_ptr<States::Storage>&& storage_manager);

  const Network::Socket& socket() override { return *socket_; }
  void registerListenerToConnectionHandler(Network::ConnectionHandler* conn_handler) override;

  class GrpcMessageImpl;

  using GenRequestFn = std::function<GrpcRequestProcessorPtr(AdminStream&)>;

  using HttpConnectionManagerProto =
      envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager;

  class GrpcRequestProcessorImpl : public GrpcRequestProcessor {
  public:

    GrpcRequestProcessorImpl(HandlerCb handler_callback, AdminStream& admin_stream);

    GrpcMessageOverHttpPtr process() override;

    static GenRequestFn defineProcessorFactory(HandlerCb callback) {
      return [callback](AdminStream& stream) -> std::unique_ptr<GrpcRequestProcessor> {
        return std::make_unique<GrpcRequestProcessorImpl>(callback, stream);
      };
    }

  private:
    HandlerCb handler_cb_;
    AdminStream& admin_stream_;
    std::unique_ptr<GrpcMessageImpl> response_msg_;
  };

  class GrpcMessageImpl : public GrpcMessageOverHttp {
  public:
    GrpcMessageImpl();
    // Http::Code writeMessageStatus(Http::Code new_code) { code_ = new_code; };
    Http::Code getMessageStatus() override { return code_; };

    // should only be executed once
    Http::ResponseHeaderMap& getMessageHeader() override { return *headers_; }
    std::unique_ptr<Http::ResponseHeaderMap> moveMessageHeader() override { return std::move(headers_); }
    std::pair<bool, std::unique_ptr<Buffer::Instance>> fetchNextMessageBodyChunk() override;

  private:
    friend class GrpcRequestProcessorImpl;
    std::unique_ptr<Http::ResponseHeaderMap> headers_;
    std::unique_ptr<Buffer::OwnedImpl> buf_;
    Http::Code code_ = Http::Code::OK;
  };

  GrpcRequestProcessorPtr matchAndApplyFactoryOnRequest(AdminStream& admin_stream) const;

  // Network::FilterChainManager
  const Network::FilterChain* findFilterChain(const Network::ConnectionSocket&,
                                              const StreamInfo::StreamInfo&) const override {
    return fast_reconfig_server_filter_chain_.get();
  }

  // Network::FilterChainFactory
  bool
  createNetworkFilterChain(Network::Connection& connection,
                           const std::vector<Network::FilterFactoryCb>& filter_factories) override;
  bool createListenerFilterChain(Network::ListenerFilterManager&) override { return true; }
  void createUdpListenerFilterChain(Network::UdpListenerFilterManager&,
                                    Network::UdpReadFilterCallbacks&) override {}

  // Http::FilterChainFactory
  bool createFilterChain(Http::FilterChainManager& manager, bool) const override;
  bool createUpgradeFilterChain(absl::string_view, const Http::FilterChainFactory::UpgradeMap*,
                                Http::FilterChainManager&) const override {
    return false;
  }

  void bindHTTPListeningSocket(Network::Address::InstanceConstSharedPtr address,
                               const Network::Socket::OptionsSharedPtr& socket_options,
                               Stats::ScopeSharedPtr&& listener_scope) override;

  // Http::ConnectionManagerConfig
  const Http::RequestIDExtensionSharedPtr& requestIDExtension() override {
    return request_id_extension_;
  }
  const std::list<AccessLog::InstanceSharedPtr>& accessLogs() override { return access_logs_; }
  const absl::optional<std::chrono::milliseconds>& accessLogFlushInterval() override {
    return null_access_log_flush_interval_;
  }
  bool flushAccessLogOnNewRequest() override { return flush_access_log_on_new_request_; }
  Http::ServerConnectionPtr createCodec(Network::Connection& connection,
                                        const Buffer::Instance& data,
                                        Http::ServerConnectionCallbacks& callbacks) override;
  Http::DateProvider& dateProvider() override { return date_provider_; }
  std::chrono::milliseconds drainTimeout() const override { return std::chrono::milliseconds(100); }
  Http::FilterChainFactory& filterFactory() override { return *this; }
  bool generateRequestId() const override { return false; }
  bool preserveExternalRequestId() const override { return false; }
  bool alwaysSetRequestIdInResponse() const override { return false; }
  absl::optional<std::chrono::milliseconds> idleTimeout() const override { return idle_timeout_; }
  bool isRoutable() const override { return false; }
  absl::optional<std::chrono::milliseconds> maxConnectionDuration() const override {
    return max_connection_duration_;
  }
  uint32_t maxRequestHeadersKb() const override { return max_request_headers_kb_; }
  uint32_t maxRequestHeadersCount() const override { return max_request_headers_count_; }
  std::chrono::milliseconds streamIdleTimeout() const override { return {}; }
  std::chrono::milliseconds requestTimeout() const override { return {}; }
  std::chrono::milliseconds requestHeadersTimeout() const override { return {}; }
  std::chrono::milliseconds delayedCloseTimeout() const override { return {}; }
  absl::optional<std::chrono::milliseconds> maxStreamDuration() const override {
    return max_stream_duration_;
  }
  Router::RouteConfigProvider* routeConfigProvider() override { return &route_config_provider_; }
  Config::ConfigProvider* scopedRouteConfigProvider() override {
    return &scoped_route_config_provider_;
  }
  const std::string& serverName() const override { return Http::DefaultServerString::get(); }
  const absl::optional<std::string>& schemeToSet() const override { return scheme_; }
  HttpConnectionManagerProto::ServerHeaderTransformation
  serverHeaderTransformation() const override {
    return HttpConnectionManagerProto::OVERWRITE;
  }
  Http::ConnectionManagerStats& stats() override { return stats_; }
  Http::ConnectionManagerTracingStats& tracingStats() override { return tracing_stats_; }
  bool useRemoteAddress() const override { return true; }
  const Http::InternalAddressConfig& internalAddressConfig() const override {
    return internal_address_config_;
  }
  uint32_t xffNumTrustedHops() const override { return 0; }
  bool skipXffAppend() const override { return false; }
  const std::string& via() const override { return EMPTY_STRING; }
  Http::ForwardClientCertType forwardClientCert() const override {
    return Http::ForwardClientCertType::Sanitize;
  }
  const std::vector<Http::ClientCertDetailsType>& setCurrentClientCertDetails() const override {
    return set_current_client_cert_details_;
  }
  const Network::Address::Instance& localAddress() override;
  const absl::optional<std::string>& userAgent() override { return user_agent_; }
  Tracing::HttpTracerSharedPtr tracer() override { return nullptr; }
  const Http::TracingConnectionManagerConfig* tracingConfig() override { return nullptr; }
  Http::ConnectionManagerListenerStats& listenerStats() override { return listener_->stats_; }
  bool proxy100Continue() const override { return false; }
  bool streamErrorOnInvalidHttpMessaging() const override { return false; }
  const Http::Http1Settings& http1Settings() const override { return http1_settings_; }
  bool shouldNormalizePath() const override { return true; }
  bool shouldMergeSlashes() const override { return true; }
  bool shouldStripTrailingHostDot() const override { return false; }
  Http::StripPortType stripPortType() const override { return Http::StripPortType::None; }
  envoy::config::core::v3::HttpProtocolOptions::HeadersWithUnderscoresAction
  headersWithUnderscoresAction() const override {
    return envoy::config::core::v3::HttpProtocolOptions::ALLOW;
  }
  const LocalReply::LocalReply& localReply() const override { return *local_reply_; }
  envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
      PathWithEscapedSlashesAction
      pathWithEscapedSlashesAction() const override {
    return envoy::extensions::filters::network::http_connection_manager::v3::HttpConnectionManager::
        KEEP_UNCHANGED;
  }
  const std::vector<Http::OriginalIPDetectionSharedPtr>&
  originalIpDetectionExtensions() const override {
    return detection_extensions_;
  }
  const std::vector<Http::EarlyHeaderMutationPtr>& earlyHeaderMutationExtensions() const override {
    return early_header_mutations_;
  }
  uint64_t maxRequestsPerConnection() const override { return 0; }
  const HttpConnectionManagerProto::ProxyStatusConfig* proxyStatusConfig() const override {
    return proxy_status_config_.get();
  }
  Http::HeaderValidatorPtr makeHeaderValidator(Http::Protocol) override {
    return nullptr;
  }
  bool appendXForwardedPort() const override { return false; }
  bool addProxyProtocolConnectionState() const override { return true; }

private:


  class ReconfigListener : public ServerEndpointListener {
  public:
    ReconfigListener(FastReconfigServerImpl& parent, Stats::ScopeSharedPtr&& listener_scope)
        : ServerEndpointListener(std::move(listener_scope),
                                 "rr_manager",
                                 parent.ignore_global_conn_limit_),
          parent_(parent)
    {}

    // Network::ListenerConfig
    Network::FilterChainManager& filterChainManager() override { return parent_; }
    Network::FilterChainFactory& filterChainFactory() override { return parent_; }
    std::vector<Network::ListenSocketFactoryPtr>& listenSocketFactories() override {
      return parent_.socket_factories_;
    }

    FastReconfigServerImpl& parent_;
  };
  using ReconfigListenerPtr = std::unique_ptr<ReconfigListener>;

  struct HandlerRegistrationItem {
    GenRequestFn processor_factory_;
  };

  static HandlerRegistrationItem registerHandler(HandlerCb handler) {
    return HandlerRegistrationItem{ GrpcRequestProcessorImpl::defineProcessorFactory(handler) };
  };

  class FastReconfigFilterChain : public ServerEndpointFilterChain {
  public:
    FastReconfigFilterChain() : ServerEndpointFilterChain() {}

    absl::string_view name() const override { return "rr_manager"; }
  };

  struct NullRouteConfigProvider : public Router::RouteConfigProvider {
    NullRouteConfigProvider(TimeSource& time_source);

    // Router::RouteConfigProvider
    Rds::ConfigConstSharedPtr config() const override { return config_; }
    const absl::optional<ConfigInfo>& configInfo() const override { return config_info_; }
    SystemTime lastUpdated() const override { return time_source_.systemTime(); }
    void onConfigUpdate() override {}
    Router::ConfigConstSharedPtr configCast() const override { return config_; }
    void requestVirtualHostsUpdate(const std::string&, Event::Dispatcher&,
                                   std::weak_ptr<Http::RouteConfigUpdatedCallback>) override {}

    Router::ConfigConstSharedPtr config_;
    absl::optional<ConfigInfo> config_info_;
    TimeSource& time_source_;
  };

  struct NullScopedRouteConfigProvider : public Config::ConfigProvider {
    NullScopedRouteConfigProvider(TimeSource &time_source)
        : config_(std::make_shared<const Router::NullScopedConfigImpl>()),
          time_source_(time_source) {}

    ~NullScopedRouteConfigProvider() override = default;

    // Config::ConfigProvider
    SystemTime lastUpdated() const override { return time_source_.systemTime(); }

    const Protobuf::Message *getConfigProto() const override { return nullptr; }

    std::string getConfigVersion() const override { return ""; }

    ConfigConstSharedPtr getConfig() const override { return config_; }

    ApiType apiType() const override { return ApiType::Full; }

    ConfigProtoVector getConfigProtos() const override { return {}; }

    Router::ScopedConfigConstSharedPtr config_;
    TimeSource &time_source_;
  };

  Server::Instance& server_;
  NullOverloadManager null_overload_manager_;
  const Network::FilterChainSharedPtr fast_reconfig_server_filter_chain_;
  ListenerHandler listener_reconfig_handler_instance_;
  ReplicateRecoverHandler replicate_recover_handler_instance_;
  Http::Http1::CodecStats::AtomicPtr http1_codec_stats_;
  Http::Http2::CodecStats::AtomicPtr http2_codec_stats_;
  Network::SocketSharedPtr socket_;
  std::map<std::string, HandlerRegistrationItem> handler_registry_;
  std::vector<Network::ListenSocketFactoryPtr> socket_factories_;
  std::unique_ptr<ReconfigListener> listener_;
  bool ignore_global_conn_limit_;
  Http::RequestIDExtensionSharedPtr request_id_extension_;
  std::list<AccessLog::InstanceSharedPtr> access_logs_;
  const absl::optional<std::chrono::milliseconds> null_access_log_flush_interval_;
  const bool flush_access_log_on_new_request_ = false;
  Http::SlowDateProviderImpl date_provider_;
  const uint32_t max_request_headers_kb_{Http::DEFAULT_MAX_REQUEST_HEADERS_KB};
  const uint32_t max_request_headers_count_{Http::DEFAULT_MAX_HEADERS_COUNT};
  absl::optional<std::chrono::milliseconds> idle_timeout_;
  absl::optional<std::chrono::milliseconds> max_connection_duration_;
  absl::optional<std::chrono::milliseconds> max_stream_duration_;
  NullRouteConfigProvider route_config_provider_;
  NullScopedRouteConfigProvider scoped_route_config_provider_;
  const absl::optional<std::string> scheme_{};
  Http::ConnectionManagerStats stats_;
  Stats::IsolatedStoreImpl no_op_store_;
  Http::ConnectionManagerTracingStats tracing_stats_;
  const FastReconfigServerInternalAddressConfig internal_address_config_;
  std::vector<Http::ClientCertDetailsType> set_current_client_cert_details_;
  absl::optional<std::string> user_agent_;
  Http::Http1Settings http1_settings_;
  const LocalReply::LocalReplyPtr local_reply_;
  const std::vector<Http::OriginalIPDetectionSharedPtr> detection_extensions_{};
  const std::vector<Http::EarlyHeaderMutationPtr> early_header_mutations_{};
  std::unique_ptr<HttpConnectionManagerProto::ProxyStatusConfig> proxy_status_config_;

};

} // namespace Server
} // namespace Envoy
