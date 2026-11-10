// Builds an artificial CameraFrame, encodes it into a datagram, prints its
// size and bytes, then decodes it and prints the received frame.

#include <comm/protocol/codec.hpp>

#include <google/protobuf/text_format.h>

#include <cstdio>
#include <string>

int main() {
  comm::protocol::CameraFrame sent;
  sent.set_protocol_version(comm::protocol::kProtocolVersion);
  sent.set_camera_id("cam_1");
  sent.set_sequence(42);
  sent.set_timestamp_ns(1762000000000000000LL);

  auto *health = sent.mutable_health();
  health->set_fps(30.0F);
  health->set_latency_ms(40);
  health->set_calibration_valid(true);

  auto *person = sent.add_persons();
  person->set_person_id("person_4");
  person->set_confidence(0.9F);
  auto *keypoint = person->add_keypoints();
  keypoint->set_x(1.5F);
  keypoint->set_y(0.0F);
  keypoint->set_z(2.0F);

  // Encode.
  std::string datagram;
  if (!comm::protocol::encode(sent, datagram)) {
    std::printf("encode failed: datagram exceeds %zu bytes\n",
                comm::protocol::kMaxDatagramSize);
    return 1;
  }

  std::printf("Datagram: %zu bytes (%zu payload + %zu CRC)\n", datagram.size(),
              datagram.size() - comm::protocol::kChecksumSize,
              comm::protocol::kChecksumSize);
  for (std::size_t i = 0; i < datagram.size(); ++i) {
    std::printf("%02X%c", static_cast<unsigned char>(datagram[i]),
                (i % 16 == 15 || i + 1 == datagram.size()) ? '\n' : ' ');
  }

  // Decode.
  comm::protocol::CameraFrame received;
  const auto error =
      comm::protocol::decode(datagram.data(), datagram.size(), received);
  std::printf("\nDecode: %s\n", comm::protocol::to_string(error));
  if (error != comm::protocol::DecodeError::None) {
    return 1;
  }

  std::printf("Protocol version: %u\n", received.protocol_version());

  std::string text;
  if (!google::protobuf::TextFormat::PrintToString(received, &text)) {
    return 1;
  }
  std::printf("\nReceived frame:\n%s", text.c_str());
  return 0;
}
