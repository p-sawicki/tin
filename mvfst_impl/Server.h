// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 06.07.2021
// Autor: Anna Pawlus

#pragma once

#include <glog/logging.h>

#include "Handler.h"
#include "LogQuicStats.h"
#include <quic/fizz/handshake/QuicFizzFactory.h>
#include <quic/server/QuicServer.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>

namespace quic {

class ServerTransportFactory : public quic::QuicServerTransportFactory {
public:
  ~ServerTransportFactory() override {
    while (!Handlers_.empty()) {
      auto &handler = Handlers_.back();
      handler->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait(
          [this] { Handlers_.pop_back(); });
    }
  }

  ServerTransportFactory() = default;

  quic::QuicServerTransport::Ptr
  make(folly::EventBase *evb, std::unique_ptr<folly::AsyncUDPSocket> sock,
       const folly::SocketAddress &,
       std::shared_ptr<const fizz::server::FizzServerContext> ctx) noexcept
      override {
    CHECK_EQ(evb, sock->getEventBase());
    auto Handler = std::make_unique<EchoHandler>(evb);
    auto transport =
        quic::QuicServerTransport::make(evb, std::move(sock), *Handler, ctx);
    Handler->setQuicSocket(transport);
    Handlers_.push_back(std::move(Handler));
    return transport;
  }

  std::vector<std::unique_ptr<EchoHandler>> Handlers_;

private:
};

class Server {
 public:
  explicit Server(const std::string& host = "::1", uint16_t port = 4436)
      : host_(host), port_(port), server_(QuicServer::createQuicServer()) {}
  
  void initQuicServer(){
    server_->setQuicServerTransportFactory(
        std::make_unique<ServerTransportFactory>());
    server_->setTransportStatsCallbackFactory(
        std::make_unique<LogQuicStatsFactory>());
    auto serverCtx = createServerCtx();
    serverCtx->setClock(std::make_shared<fizz::SystemClock>());
    server_->setFizzContext(serverCtx);
  }

  folly::ssl::EvpPkeyUniquePtr getPrivateKey(folly::StringPiece key) {
    folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
    BIO_write(bio.get(), key.data(), key.size());
    folly::ssl::EvpPkeyUniquePtr pkey(
        PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr));
    return pkey;
  }

  folly::ssl::X509UniquePtr getCert(folly::StringPiece cert) {
    folly::ssl::BioUniquePtr bio(BIO_new(BIO_s_mem()));
    BIO_write(bio.get(), cert.data(), cert.size());
    folly::ssl::X509UniquePtr x509(
        PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    return x509;
  }

  std::shared_ptr<fizz::SelfCert> readCert() {
    auto certificate = this->getCert(this->kP256Certificate);
    auto privKey = this->getPrivateKey(this->kP256Key);
    std::vector<folly::ssl::X509UniquePtr> certs;
    certs.emplace_back(std::move(certificate));
    return std::make_shared<fizz::SelfCertImpl<fizz::KeyType::P256>>(
        std::move(privKey), std::move(certs));
  }

  std::shared_ptr<fizz::server::FizzServerContext> createServerCtx() {
    auto cert = this->readCert();
    auto certManager = std::make_unique<fizz::server::CertManager>();
    certManager->addCert(std::move(cert), true);

    auto serverCtx = std::make_shared<fizz::server::FizzServerContext>();

    serverCtx->setFactory(std::make_shared<QuicFizzFactory>());
    serverCtx->setCertManager(std::move(certManager));
    serverCtx->setOmitEarlyRecordLayer(true);
    serverCtx->setClock(std::make_shared<fizz::SystemClock>());
    return serverCtx;
  }

  void runQuicServer() {
    // Create a SocketAddress and the default or passed in host.
    folly::SocketAddress addr1(host_.c_str(), port_);
    addr1.setFromHostPort(host_, port_);
    server_->start(addr1, 0);
    LOG(INFO) << "Server started at: " << addr1.describe();
    eventbase_.loopForever();
  }
  

private:
  std::string host_;
  uint16_t port_;
  folly::EventBase eventbase_;
  std::shared_ptr<quic::QuicServer> server_;

  folly::StringPiece kP256Key = R"(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIHMPeLV/nP/gkcgU2weiXl198mEX8RbFjPRoXuGcpxMXoAoGCCqGSM49
AwEHoUQDQgAEnYe8rdtl2Nz234sUipZ5tbcQ2xnJWput//E0aMs1i04h0kpcgmES
ZY67ltZOKYXftBwZSDNDkaSqgbZ4N+Lb8A==
-----END EC PRIVATE KEY-----
    )";

  folly::StringPiece kP256Certificate = R"(
-----BEGIN CERTIFICATE-----
MIIB7jCCAZWgAwIBAgIJAMVp7skBzobZMAoGCCqGSM49BAMCMFQxCzAJBgNVBAYT
AlVTMQswCQYDVQQIDAJOWTELMAkGA1UEBwwCTlkxDTALBgNVBAoMBEZpenoxDTAL
BgNVBAsMBEZpenoxDTALBgNVBAMMBEZpenowHhcNMTcwNDA0MTgyOTA5WhcNNDEx
MTI0MTgyOTA5WjBUMQswCQYDVQQGEwJVUzELMAkGA1UECAwCTlkxCzAJBgNVBAcM
Ak5ZMQ0wCwYDVQQKDARGaXp6MQ0wCwYDVQQLDARGaXp6MQ0wCwYDVQQDDARGaXp6
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEnYe8rdtl2Nz234sUipZ5tbcQ2xnJ
Wput//E0aMs1i04h0kpcgmESZY67ltZOKYXftBwZSDNDkaSqgbZ4N+Lb8KNQME4w
HQYDVR0OBBYEFDxbi6lU2XUvrzyK1tGmJEncyqhQMB8GA1UdIwQYMBaAFDxbi6lU
2XUvrzyK1tGmJEncyqhQMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIg
NJt9NNcTL7J1ZXbgv6NsvhcjM3p6b175yNO/GqfvpKUCICXFCpHgqkJy8fUsPVWD
p9fO4UsXiDUnOgvYFDA+YtcU
-----END CERTIFICATE-----
    )";
  };
} // namespace quic