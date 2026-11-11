// Sends datagrams over 127.0.0.1 and checks what arrives: a full
// encode -> send -> receive -> decode round trip, timeouts, truncation, and
// corruption caught by the protocol's CRC.

#include "check.hpp"

#include <comm/codec.hpp>
#include <comm/udp.hpp>

#include <array>
#include <chrono>
#include <string>

namespace {

using comm::ReceiveStatus;
using std::chrono::milliseconds;

constexpr milliseconds kWait{1000};

comm::CameraFrame sample_frame() {
  comm::CameraFrame frame;
  frame.set_protocol_version(comm::kProtocolVersion);
  frame.set_camera_id("cam_1");
  frame.set_sequence(42);
  frame.set_timestamp_ns(1762000000000000000LL);
  frame.mutable_health()->set_fps(30.0F);
  auto *person = frame.add_persons();
  person->set_person_id("person_4");
  person->set_confidence(0.9F);
  return frame;
}

void round_trip() {
  comm::UdpReceiver receiver(0, "127.0.0.1");
  comm::UdpSender sender("127.0.0.1", receiver.port());

  std::string datagram;
  CHECK(comm::encode(sample_frame(), datagram));
  CHECK(sender.send(datagram));

  std::array<char, comm::kMaxDatagramSize> buffer{};
  const auto result = receiver.receive(buffer.data(), buffer.size(), kWait);
  CHECK(result.status == ReceiveStatus::Ok);
  CHECK(result.size == datagram.size());

  comm::CameraFrame received;
  CHECK(comm::decode(buffer.data(), result.size, received) ==
        comm::DecodeError::None);
  CHECK(received.camera_id() == "cam_1");
  CHECK(received.sequence() == 42);
  CHECK(received.persons_size() == 1);
}

void timeout_when_nothing_arrives() {
  comm::UdpReceiver receiver(0, "127.0.0.1");
  std::array<char, 64> buffer{};
  const auto start = std::chrono::steady_clock::now();
  const auto result =
      receiver.receive(buffer.data(), buffer.size(), milliseconds{50});
  const auto elapsed = std::chrono::steady_clock::now() - start;
  CHECK(result.status == ReceiveStatus::Timeout);
  CHECK(elapsed >= milliseconds{40});
}

void oversized_datagram_is_truncated() {
  comm::UdpReceiver receiver(0, "127.0.0.1");
  comm::UdpSender sender("127.0.0.1", receiver.port());
  const std::string datagram(64, 'x');
  CHECK(sender.send(datagram));
  std::array<char, 16> buffer{};
  const auto result = receiver.receive(buffer.data(), buffer.size(), kWait);
  CHECK(result.status == ReceiveStatus::Truncated);
}

void corruption_is_rejected_by_decode() {
  comm::UdpReceiver receiver(0, "127.0.0.1");
  comm::UdpSender sender("127.0.0.1", receiver.port());
  std::string datagram;
  CHECK(comm::encode(sample_frame(), datagram));
  datagram[0] = static_cast<char>(datagram[0] ^ 0x01);
  CHECK(sender.send(datagram));

  std::array<char, comm::kMaxDatagramSize> buffer{};
  const auto result = receiver.receive(buffer.data(), buffer.size(), kWait);
  CHECK(result.status == ReceiveStatus::Ok);
  comm::CameraFrame received;
  CHECK(comm::decode(buffer.data(), result.size, received) ==
        comm::DecodeError::BadChecksum);
}

} // namespace

int main() {
  round_trip();
  timeout_when_nothing_arrives();
  oversized_datagram_is_truncated();
  corruption_is_rejected_by_decode();
  return test::finish("udp_loopback_test");
}
