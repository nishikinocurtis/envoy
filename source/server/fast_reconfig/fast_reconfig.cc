//
// Created by curtis on 7/6/23.
//



#include "fast_reconfig.h"
#include "source/common/http/conn_manager_utility.h"
#include "source/common/network/listen_socket_impl.h"

#include "source/extensions/request_id/uuid/config.h"

namespace Envoy {
namespace Server {

void FastReconfigServerImpl::bindHTTPListeningSocket(Network::Address::InstanceConstSharedPtr address,
                                                     const Network::Socket::OptionsSharedPtr &socket_options,
                                                     Stats::ScopeSharedPtr &&listener_scope) {
  null_overload_manager_.start();
  socket_ = std::make_shared<Network::TcpListenSocket>(address, socket_options, true);
  RELEASE_ASSERT(0 == socket_->ioHandle().listen(ENVOY_TCP_BACKLOG_SIZE).return_value_,
                 "listen() failed on re-config listener");
  socket_factories_.emplace_back(std::make_unique<ServerEndpointListenSocketFactory>(socket_));
  listener_ = std::make_unique<ReconfigListener>(*this, std::move(listener_scope));
  ENVOY_LOG(info, "admin address: {}",
            socket().connectionInfoProvider().localAddress()->asString());
  // dump logs..
}

using GrpcMessageImpl = FastReconfigServerImpl::GrpcMessageImpl;
using GrpcRequestProcessorImpl = FastReconfigServerImpl::GrpcRequestProcessorImpl;

FastReconfigServerImpl::FastReconfigServerImpl(Server::Instance& server,
                                               bool ignore_global_conn_limit,
                                               SubscriptionCallbackWeakPtr&& listener_reconfig_callback,
                                               Config::OpaqueResourceDecoderSharedPtr&& listener_reconfig_resource_decoder,
                                               std::shared_ptr<States::Storage>&& storage_manager)
    : server_(server),
      null_overload_manager_(server_.threadLocal()),
      fast_reconfig_server_filter_chain_(std::make_shared<FastReconfigFilterChain>()),
      listener_reconfig_handler_instance_(server_,
                                          std::move(listener_reconfig_callback),
                                          std::move(listener_reconfig_resource_decoder)),
      replicate_recover_handler_instance_(server_,
                                          std::move(storage_manager)),
      handler_registry_({
        {"/rr_listener", registerHandler(HANDLER_WRAPPER(listener_reconfig_handler_instance_.pushNewListenersHandler))},
        {"/replicate_single", registerHandler(HANDLER_WRAPPER(replicate_recover_handler_instance_.onStatesReplication))},
        {"/failure_single", registerHandler(HANDLER_WRAPPER(replicate_recover_handler_instance_.onFailureRecovery))}}),
      ignore_global_conn_limit_(ignore_global_conn_limit),
      request_id_extension_(Extensions::RequestId::UUIDRequestIDExtension::defaultInstance(
        server_.api().randomGenerator())), date_provider_(server.dispatcher().timeSource()),
      route_config_provider_(server.timeSource()),
      scoped_route_config_provider_(server.timeSource()),
      stats_(Http::ConnectionManagerImpl::generateStats(
          "http.rr.", *server_.stats().rootScope())),
      tracing_stats_(Http::ConnectionManagerImpl::generateTracingStats("http.rr.",
                                                                       *no_op_store_.rootScope()))
{ }

GrpcMessageImpl::GrpcMessageImpl() {
  headers_ = Http::ResponseHeaderMapImpl::create();
  buf_ = std::make_unique<Buffer::OwnedImpl>();
}

std::pair<bool, std::unique_ptr<Buffer::Instance>> GrpcMessageImpl::fetchNextMessageBodyChunk() {
  return std::make_pair(false, std::move(buf_));
}

GrpcRequestProcessorImpl::
    GrpcRequestProcessorImpl(FastReconfigServer::HandlerCb handler_callback,
                             AdminStream& admin_stream)
    : handler_cb_(handler_callback),
      admin_stream_(admin_stream),
      response_msg_(std::make_unique<GrpcMessageImpl>()) {}

FastReconfigServer::GrpcMessageOverHttpPtr GrpcRequestProcessorImpl::process() {
  response_msg_->code_ =
      handler_cb_(admin_stream_, *response_msg_->headers_, *response_msg_->buf_);
  return std::move(response_msg_);
}

FastReconfigServer::GrpcRequestProcessorPtr FastReconfigServerImpl::matchAndApplyFactoryOnRequest(AdminStream &admin_stream) const {
  // get request path
  // fetch factory from registry
  // apply factory with admin_stream, and return the processor.
  auto iter = handler_registry_.find("/rr_listener");
  if (iter != handler_registry_.end()) {
    return iter->second.processor_factory_(admin_stream);
  } else {

  }
}

bool FastReconfigServerImpl::createNetworkFilterChain(Network::Connection &connection,
                                                      const std::vector<Network::FilterFactoryCb> &filter_factories) {
  // adopted from AdminImpl
  connection.addReadFilter(Network::ReadFilterSharedPtr{new Http::ConnectionManagerImpl(
      *this, server_.drainManager(), server_.api().randomGenerator(), server_.httpContext(),
      server_.runtime(), server_.localInfo(), server_.clusterManager(), null_overload_manager_,
      server_.timeSource())});
  return true;
}

bool FastReconfigServerImpl::createFilterChain(Http::FilterChainManager &manager, bool) const {
  Http::FilterFactoryCb factory = [this](Http::FilterChainFactoryCallbacks& callbacks) {
    callbacks.addStreamFilter(std::make_shared<FastReconfigFilter>(
        [this](AdminStream& admin_stream) { return matchAndApplyFactoryOnRequest(admin_stream); }
        ));
  };
  manager.applyFilterFactoryCb({}, factory);
  return true;
}

void FastReconfigServerImpl::registerListenerToConnectionHandler(Network::ConnectionHandler* conn_handler) {
  if (listener_) {
    conn_handler->addListener(absl::nullopt, *listener_, server_.runtime());
  }
}

Http::ServerConnectionPtr FastReconfigServerImpl::createCodec(Network::Connection& connection,
                                                 const Buffer::Instance& data,
                                                 Http::ServerConnectionCallbacks& callbacks) {
  return Http::ConnectionManagerUtility::autoCreateCodec(
      connection, data, callbacks, *server_.stats().rootScope(), server_.api().randomGenerator(),
      http1_codec_stats_, http2_codec_stats_, Http::Http1Settings(),
      ::Envoy::Http2::Utility::initializeAndValidateOptions(
          envoy::config::core::v3::Http2ProtocolOptions()),
      maxRequestHeadersKb(), maxRequestHeadersCount(), headersWithUnderscoresAction());
}

const Network::Address::Instance& FastReconfigServerImpl::localAddress() {
  return *server_.localInfo().address();
}


}
}