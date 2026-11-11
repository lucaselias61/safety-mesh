#include <comm/codec.hpp>

#include <array>

namespace comm {
namespace {

const std::array<std::uint32_t, 256> &crc_table() {
  static const auto table = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < t.size(); ++i) {
      std::uint32_t c = i;
      for (int bit = 0; bit < 8; ++bit) {
        c = (c & 1U) ? 0xEDB88320U ^ (c >> 1) : c >> 1;
      }
      t[i] = c;
    }
    return t;
  }();
  return table;
}

void append_be32(std::string &out, std::uint32_t value) {
  out.push_back(static_cast<char>(value >> 24));
  out.push_back(static_cast<char>(value >> 16));
  out.push_back(static_cast<char>(value >> 8));
  out.push_back(static_cast<char>(value));
}

std::uint32_t read_be32(const unsigned char *p) {
  return (std::uint32_t{p[0]} << 24) | (std::uint32_t{p[1]} << 16) |
         (std::uint32_t{p[2]} << 8) | std::uint32_t{p[3]};
}

} // namespace

const char *to_string(DecodeError error) {
  switch (error) {
  case DecodeError::None:
    return "none";
  case DecodeError::TooShort:
    return "datagram too short";
  case DecodeError::BadChecksum:
    return "checksum mismatch";
  case DecodeError::Malformed:
    return "malformed payload";
  case DecodeError::UnsupportedVersion:
    return "unsupported protocol version";
  }
  return "unknown";
}

std::uint32_t crc32(const void *data, std::size_t size) {
  const auto &table = crc_table();
  const auto *bytes = static_cast<const unsigned char *>(data);
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t i = 0; i < size; ++i) {
    crc = table[(crc ^ bytes[i]) & 0xFFU] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFU;
}

bool encode(const CameraFrame &frame, std::string &out) {
  out.clear();
  if (!frame.SerializeToString(&out)) {
    return false;
  }
  append_be32(out, crc32(out.data(), out.size()));
  return out.size() <= kMaxDatagramSize;
}

DecodeError decode(const void *data, std::size_t size, CameraFrame &out) {
  if (size < kChecksumSize) {
    return DecodeError::TooShort;
  }
  const auto *bytes = static_cast<const unsigned char *>(data);
  const std::size_t payload_size = size - kChecksumSize;
  if (crc32(bytes, payload_size) != read_be32(bytes + payload_size)) {
    return DecodeError::BadChecksum;
  }
  if (!out.ParseFromArray(bytes, static_cast<int>(payload_size))) {
    return DecodeError::Malformed;
  }
  if (out.protocol_version() != kProtocolVersion) {
    return DecodeError::UnsupportedVersion;
  }
  return DecodeError::None;
}

} // namespace comm
