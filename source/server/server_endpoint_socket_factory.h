//
// Created by qiutong on 7/10/23.
//

#include "envoy/network/listen_socket.h"
#include "envoy/network/listener.h"

#include "source/common/common/assert.h"

namespace Envoy {
namespace Server {

class ServerEndpointListenSocketFactory : public Network::ListenSocketFactory {
public:
  ServerEndpointListenSocketFactory(Network::SocketSharedPtr socket) : socket_(socket) {}

  // Network::ListenSocketFactory
  Network::Socket::Type socketType() const override { return socket_->socketType(); }
  const Network::Address::InstanceConstSharedPtr& localAddress() const override {
    return socket_->connectionInfoProvider().localAddress();
  }
  Network::SocketSharedPtr getListenSocket(uint32_t) override {
    // This is only supposed to be called once.
    RELEASE_ASSERT(!socket_create_, "Endpoint server's socket shouldn't be shared.");
    socket_create_ = true;
    return socket_;
  }
  Network::ListenSocketFactoryPtr clone() const override { return nullptr; }
  void closeAllSockets() override {}
  void doFinalPreWorkerInit() override {}

protected:
  Network::SocketSharedPtr socket_;
  bool socket_create_{false};
};

}
}