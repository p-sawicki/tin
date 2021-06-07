#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "LogQuicStats.h"
#include "utils.h"
#include <folly/fibers/Baton.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <glog/logging.h>
#include <quic/api/QuicSocket.h>
#include <quic/client/QuicClientTransport.h>
#include <quic/common/BufUtil.h>
#include <quic/fizz/client/handshake/FizzClientQuicHandshakeContext.h>

namespace quic {

class TestCertificateVerifier : public fizz::CertificateVerifier {
public:
  ~TestCertificateVerifier() override = default;

  void verify(const std::vector<std::shared_ptr<const fizz::PeerCert>> &)
      const override {
    return;
  }

  std::vector<fizz::Extension>
  getCertificateRequestExtensions() const override {
    return std::vector<fizz::Extension>();
  }
};

class Client : public quic::QuicSocket::ConnectionCallback,
               public quic::QuicSocket::ReadCallback,
               public quic::QuicSocket::WriteCallback {
public:
  Client(const std::string &host, uint16_t port) : host_(host), port_(port), netThread("ClientThread") {}

  void readAvailable(quic::StreamId streamId) noexcept override {
    auto readData = quicClient_->read(streamId, 0);
    if (readData.hasError()) {
      LOG(ERROR) << "Client failed read from stream=" << streamId
                 << ", error=" << (uint32_t)readData.error();
    }
    auto copy = readData->first->clone();
    if (recvOffsets_.find(streamId) == recvOffsets_.end()) {
      recvOffsets_[streamId] = copy->length();
    } else {
      recvOffsets_[streamId] += copy->length();
    }
  }

  void readError(
      quic::StreamId streamId,
      std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
          error) noexcept override {
    LOG(ERROR) << "Client failed read from stream=" << streamId
               << ", error=" << toString(error);
  }

  void onNewBidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Client: new bidirectional stream=" << id;
    quicClient_->setReadCallback(id, this);
  }

  void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Client: new unidirectional stream=" << id;
    quicClient_->setReadCallback(id, this);
  }

  void onStopSending(
      quic::StreamId id,
      quic::ApplicationErrorCode /*error*/) noexcept override {
      VLOG(10) << "Client got StopSending stream id=" << id;
  }

  void onConnectionEnd() noexcept override {
      LOG(INFO) << "Client connection end";
  }

  void onConnectionError(
      std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
      LOG(ERROR) << "Client error: " << toString(error.first)
                 << "; errStr=" << error.second;
    startDone_.post();
  }

  void onTransportReady() noexcept override { startDone_.post(); }

  void onStreamWriteReady(quic::StreamId id,
                          uint64_t maxToSend) noexcept override {
    LOG(INFO) << "Client socket is write ready with maxToSend=" << maxToSend;
    sendMessage(id, pendingOutput_[id]);
  }

  void onStreamWriteError(
      quic::StreamId id,
      std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
          error) noexcept override {
    LOG(ERROR) << "Client write error with stream=" << id
               << " error=" << toString(error);
  }

  std::unique_ptr<fizz::CertificateVerifier> createTestCertificateVerifier() {
    return std::make_unique<TestCertificateVerifier>();
  }

  void initQuicClient() {
    auto evb = netThread.getEventBase();
    folly::SocketAddress addr(host_.c_str(), port_);
    
    evb->runInEventBaseThreadAndWait([&] {
      auto sock = std::make_unique<folly::AsyncUDPSocket>(evb);
      auto fizzClientContext =
          FizzClientQuicHandshakeContext::Builder()
              .setCertificateVerifier(this->createTestCertificateVerifier())
              .build();
      quicClient_ = std::make_shared<quic::QuicClientTransport>(
          evb, std::move(sock), std::move(fizzClientContext));
      quicClient_->setHostname("echo.com");
      quicClient_->addNewPeerAddress(addr);

      TransportSettings settings;
      quicClient_->setTransportSettings(settings);
      quicClient_->setTransportStatsCallback(
          std::make_shared<LogQuicStats>("client"));

      LOG(INFO) << "Client connecting to " << addr.describe();
      quicClient_->start(this);
    });
    startDone_.wait();
    
  }

void runQuicClient(int ifDelay, int scenario) {
    auto evb1 = netThread.getEventBase();
    auto client = quicClient_;
    //auto evb = networkThread.getEventBase();
    int nbStreams = (unsigned int[]){1, 1, 10, 10}[scenario - 1];
    std::string message = std::to_string(scenario);

    for(int i = 0; i < nbStreams; i++) {
      evb1->runInEventBaseThreadAndWait([=] {
        // create new stream for each message
        auto streamId = client->createBidirectionalStream().value();
        client->setReadCallback(streamId, this);
        pendingOutput_[streamId].append(folly::IOBuf::copyBuffer(message));
        sendMessage(streamId, pendingOutput_[streamId]);
      });
      if (nbStreams > 1) 
          sleep(6);
      
    }

    sleep(2);
    LOG(INFO) << "Client stopping client.";
  }

  ~Client() override = default;

private:
  void sendMessage(quic::StreamId id, BufQueue &data) {
    auto message = data.move();
    auto res = quicClient_->writeChain(id, message->clone(), true);
    if (res.hasError()) {
      LOG(ERROR) << "Client writeChain error=" << uint32_t(res.error());
    } else {
      auto str = message->moveToFbString().toStdString();
      LOG(INFO) << "Client sending request, scenario "<< str << " on stream=" << id;
      pendingOutput_.erase(id);
    }
  }

  std::string host_;
  uint16_t port_;
  std::shared_ptr<quic::QuicClientTransport> quicClient_;
  std::map<quic::StreamId, BufQueue> pendingOutput_;
  std::map<quic::StreamId, uint64_t> recvOffsets_;
  folly::fibers::Baton startDone_;
  folly::ScopedEventBaseThread netThread;
};
} // namespace quic