// TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
// Utworzono: 06.07.2021
// Autor: Anna Pawlus

#pragma once

#include <folly/io/async/EventBase.h>
#include <quic/api/QuicSocket.h>
#include <quic/common/BufUtil.h>

namespace quic {
class EchoHandler : public quic::QuicSocket::ConnectionCallback,
                    public quic::QuicSocket::ReadCallback,
                    public quic::QuicSocket::WriteCallback {
public:
  using StreamData = std::pair<BufQueue, bool>;

  explicit EchoHandler(folly::EventBase *evbIn) : evb(evbIn) {}

  void setQuicSocket(std::shared_ptr<quic::QuicSocket> socket) {
    sock = socket;
  }

  void onNewBidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Got bidirectional stream id=" << id;
    sock->setReadCallback(id, this);
  }

  void onNewUnidirectionalStream(quic::StreamId id) noexcept override {
    LOG(INFO) << "Got unidirectional stream id=" << id;
    sock->setReadCallback(id, this);
  }

  void onStopSending(quic::StreamId id,
                     quic::ApplicationErrorCode error) noexcept override {
    LOG(INFO) << "Got StopSending stream id=" << id << " error=" << error;
  }

  void onConnectionEnd() noexcept override { LOG(INFO) << "Socket closed"; }

  void onConnectionError(
      std::pair<quic::QuicErrorCode, std::string> error) noexcept override {
    //LOG(ERROR) << "Socket error=" << toString(error.first);
  }

  void readAvailable(quic::StreamId id) noexcept override {
    LOG(INFO) << "read available for stream id=" << id;

    auto res = sock->read(id, 0);
    if (res.hasError()) {
      LOG(ERROR) << "Got error=" << toString(res.error());
      return;
    }
    if (input_.find(id) == input_.end()) {
      input_.emplace(id, std::make_pair(BufQueue(), false));
    }

    quic::Buf data = std::move(res.value().first);
    bool eof = res.value().second;
    auto dataRec = ((data) ? data->clone()->moveToFbString().toStdString() : std::string());
    auto scenario = std::stoi(dataRec);
    int packetSize = (unsigned int[]) {100, 107374824, 100, 10485760}[scenario - 1];
    LOG(INFO)<<"Received request scenario = "<<scenario;

    input_[id].first.append(std::move(data));
    input_[id].second = eof;

    if (eof) {
      sendData(id, input_[id], packetSize);
    }
  }

  void sendData(quic::StreamId id, StreamData &data, int packetSize) {
    std::string packet(packetSize, 'x');
    auto dataPacket = folly::IOBuf::copyBuffer(packet);
    dataPacket->prependChain(data.first.move());

    auto res = sock->writeChain(id, std::move(dataPacket), true, nullptr);
    if (res.hasError()) {
      LOG(ERROR) << "write error=" << toString(res.error());
    } else {
      data.second = false;
    }
  }

  void readError(quic::StreamId id,
          std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
              error) noexcept override {
  LOG(ERROR) << "Got read error on stream=" << id
              << " error=" << toString(error);
  }

  void onStreamWriteReady(quic::StreamId id,
                          uint64_t maxToSend) noexcept override {
    LOG(INFO) << "socket is write ready with maxToSend=" << maxToSend;
  }

  void onStreamWriteError(
      quic::StreamId id,
      std::pair<quic::QuicErrorCode, folly::Optional<folly::StringPiece>>
          error) noexcept override {
    LOG(ERROR) << "write error with stream=" << id
               << " error=" << toString(error);
  }

  folly::EventBase *getEventBase() { return evb; }

  folly::EventBase *evb;
  std::shared_ptr<quic::QuicSocket> sock;

private:
  std::map<quic::StreamId, StreamData> input_;
};
} // namespace quic