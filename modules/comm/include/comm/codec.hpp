#pragma once

#include <comm/camera_frame.pb.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace comm {

// Bump when the datagram layout changes incompatibly.
inline constexpr std::uint32_t kProtocolVersion = 1;

// Stay below a typical 1500-byte Ethernet MTU to avoid IP fragmentation.
inline constexpr std::size_t kMaxDatagramSize = 1400;

// Datagram layout: [protobuf payload][CRC-32 of payload, 4 bytes big-endian].
inline constexpr std::size_t kChecksumSize = 4;

enum class DecodeError {
  None,
  TooShort,
  BadChecksum,
  Malformed,
  UnsupportedVersion,
};

const char *to_string(DecodeError error);

// Serializes `frame` into `out` (reused to avoid per-packet allocations).
// Returns false if the datagram would exceed kMaxDatagramSize.
bool encode(const CameraFrame &frame, std::string &out);

// Validates and parses one received datagram into `out`.
DecodeError decode(const void *data, std::size_t size, CameraFrame &out);

// CRC-32 (IEEE 802.3), exposed for tests and for non-protobuf telegrams.
std::uint32_t crc32(const void *data, std::size_t size);

} // namespace comm
