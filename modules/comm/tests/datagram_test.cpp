// Checks the datagram codec: round trip, byte layout, CRC-32, size limit and
// every decode error. Needs no network.

#include "check.hpp"

#include <comm/protocol/codec.hpp>

#include <google/protobuf/util/message_differencer.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace {

namespace protocol = comm::protocol;
using protocol::DecodeError;

protocol::CameraFrame sample_frame() {
  protocol::CameraFrame frame;
  frame.set_protocol_version(protocol::kProtocolVersion);
  frame.set_camera_id("cam_1");
  frame.set_sequence(42);
  frame.set_timestamp_ns(1762000000000000000LL);

  auto *health = frame.mutable_health();
  health->set_fps(30.0F);
  health->set_latency_ms(40);
  health->set_calibration_valid(true);

  auto *person = frame.add_persons();
  person->set_person_id("person_4");
  person->set_confidence(0.9F);
  auto *keypoint = person->add_keypoints();
  keypoint->set_x(1.5F);
  keypoint->set_y(0.0F);
  keypoint->set_z(2.0F);
  return frame;
}

// Appends a valid big-endian CRC-32, as encode does, to arbitrary bytes.
std::string with_checksum(std::string payload) {
  const std::uint32_t crc = protocol::crc32(payload.data(), payload.size());
  for (const int shift : {24, 16, 8, 0})
    payload.push_back(static_cast<char>(crc >> shift));
  return payload;
}

DecodeError decode(const std::string &datagram) {
  protocol::CameraFrame frame;
  return protocol::decode(datagram.data(), datagram.size(), frame);
}

void round_trip_preserves_every_field() {
  const auto sent = sample_frame();
  std::string datagram;
  CHECK(protocol::encode(sent, datagram));

  protocol::CameraFrame received;
  CHECK(protocol::decode(datagram.data(), datagram.size(), received) ==
        DecodeError::None);
  CHECK(google::protobuf::util::MessageDifferencer::Equals(sent, received));
}

void datagram_is_payload_then_big_endian_crc() {
  const auto frame = sample_frame();
  std::string datagram;
  CHECK(protocol::encode(frame, datagram));

  const std::string payload = frame.SerializeAsString();
  CHECK(datagram.size() == payload.size() + protocol::kChecksumSize);
  CHECK(datagram.compare(0, payload.size(), payload) == 0);
  CHECK(datagram == with_checksum(payload));
}

void crc32_matches_standard_check_value() {
  // CRC-32/IEEE 802.3 check value for "123456789".
  CHECK(protocol::crc32("123456789", 9) == 0xCBF43926U);
  CHECK(protocol::crc32("", 0) == 0U);
}

void encode_replaces_previous_buffer_contents() {
  std::string fresh;
  CHECK(protocol::encode(sample_frame(), fresh));
  std::string reused = "stale bytes from an earlier datagram";
  CHECK(protocol::encode(sample_frame(), reused));
  CHECK(reused == fresh);
}

void encode_rejects_frames_over_the_size_limit() {
  auto frame = sample_frame();
  while (frame.ByteSizeLong() <= protocol::kMaxDatagramSize)
    frame.add_persons()->set_person_id(std::string(100, 'p'));
  std::string datagram;
  CHECK(!protocol::encode(frame, datagram));
}

void decode_rejects_datagrams_shorter_than_the_crc() {
  for (std::size_t size = 0; size < protocol::kChecksumSize; ++size)
    CHECK(decode(std::string(size, '\0')) == DecodeError::TooShort);
}

void decode_rejects_any_single_bit_error() {
  std::string datagram;
  CHECK(protocol::encode(sample_frame(), datagram));
  // Covers payload and CRC bytes; CRC-32 detects every single-bit error.
  for (std::size_t i = 0; i < datagram.size(); ++i) {
    for (int bit = 0; bit < 8; ++bit) {
      std::string corrupted = datagram;
      corrupted[i] = static_cast<char>(corrupted[i] ^ (1 << bit));
      CHECK(decode(corrupted) == DecodeError::BadChecksum);
    }
  }
}

void decode_rejects_unparseable_payload_with_valid_crc() {
  // 0xFF starts a varint that never ends.
  CHECK(decode(with_checksum("\xFF")) == DecodeError::Malformed);
}

void decode_rejects_other_protocol_versions() {
  auto frame = sample_frame();
  frame.set_protocol_version(protocol::kProtocolVersion + 1);
  std::string datagram;
  CHECK(protocol::encode(frame, datagram));
  CHECK(decode(datagram) == DecodeError::UnsupportedVersion);

  // An empty payload parses as an all-default frame with version 0.
  CHECK(decode(with_checksum("")) == DecodeError::UnsupportedVersion);
}

void every_error_has_a_distinct_description() {
  const DecodeError errors[] = {
      DecodeError::None, DecodeError::TooShort, DecodeError::BadChecksum,
      DecodeError::Malformed, DecodeError::UnsupportedVersion};
  for (const auto a : errors) {
    CHECK(std::strcmp(protocol::to_string(a), "unknown") != 0);
    for (const auto b : errors)
      if (a != b)
        CHECK(std::strcmp(protocol::to_string(a), protocol::to_string(b)) != 0);
  }
}

} // namespace

int main() {
  round_trip_preserves_every_field();
  datagram_is_payload_then_big_endian_crc();
  crc32_matches_standard_check_value();
  encode_replaces_previous_buffer_contents();
  encode_rejects_frames_over_the_size_limit();
  decode_rejects_datagrams_shorter_than_the_crc();
  decode_rejects_any_single_bit_error();
  decode_rejects_unparseable_payload_with_valid_crc();
  decode_rejects_other_protocol_versions();
  every_error_has_a_distinct_description();
  return test::finish("datagram_test");
}
