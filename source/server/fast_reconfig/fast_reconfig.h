//
// Created by curtis on 6/30/23.
//

#pragma once

#include "envoy/network/filter.h"
#include "envoy/server/fast_reconfig.h"
#include "envoy/server/instance.h"
#include "envoy/server/listener_manager.h"
#include "envoy/network/listen_socket.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/http/conn_manager_config.h"
#include "source/common/http/conn_manager_impl.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/server/fast_reconfig/listener_handler.h"
#include "source/server/server_endpoint_listener.h"
#include "source/server/server_endpoint_filter.h"
#include "source/server/fast_reconfig/fast_reconfig_filter.h"

#include "source/extensions/listener_managers/listener_manager/lds_api.h"

#include "source/common/common/logger.h"

namespace Envoy {
namespace Server {

class FastReconfigServerImpl : public FastReconfigServer,
                               public Network::FilterChainManager,
                               public Network::FilterChainFactory,
                               public Http::FilterChainFactory,
                               public Http::ConnectionManagerConfig,
                               Logger::Loggable<Logger::Id::rr_manager> {
public:
  FastReconfigServerImpl(Server::Instance& server,
                         bool ignore_global_conn_limit,
                         LdsApiImpl& listener_reconfig_callback);

  class GrpcMessageImpl;

  using GenRequestFn = std::function<GrpcRequestProcessorPtr(AdminStream&)>;

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
    std::unique_ptr<Http::ResponseHeaderMap> dumpMessageHeader() override { return std::move(headers_); } ;
    std::pair<bool, std::unique_ptr<Buffer::Instance>> fetchNextMessageBodyChunk() override;

  private:
    friend class GrpcRequestProcessorImpl;
    std::unique_ptr<Http::ResponseHeaderMapImpl> headers_;
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

  Server::Instance& server_;
  const Network::FilterChainSharedPtr fast_reconfig_server_filter_chain_;
  ListenerHandler listener_reconfig_handler_instance_;
  Network::SocketSharedPtr socket_;
  std::map<std::string, HandlerRegistrationItem> handler_registry_;
  std::vector<Network::ListenSocketFactoryPtr> socket_factories_;
  bool ignore_global_conn_limit_;

};

} // namespace Server
} // namespace Envoy
