#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// UDP datagrams as raw bytes. Knows nothing about the message format; pair
// with comm/codec.hpp to encode and decode. UDP may drop, duplicate or reorder
// datagrams, so receivers must check sequence numbers and timestamps.
namespace comm {

// Sends datagrams to one fixed destination. Not thread-safe.
class UdpSender {
public:
  // Resolves `host` (a name or an IPv4/IPv6 address). Throws
  // std::runtime_error if it cannot be resolved, std::system_error otherwise.
  UdpSender(const std::string &host, std::uint16_t port);
  ~UdpSender();

  UdpSender(UdpSender &&other) noexcept;
  UdpSender &operator=(UdpSender &&other) noexcept;
  UdpSender(const UdpSender &) = delete;
  UdpSender &operator=(const UdpSender &) = delete;

  // Hands one datagram to the OS. Returns false if it was rejected, for
  // example with no route, a full buffer, or an ICMP error reported for an
  // earlier datagram (often: nothing listening). True does not mean delivered.
  bool send(const void *data, std::size_t size);
  bool send(std::string_view datagram) {
    return send(datagram.data(), datagram.size());
  }

private:
  int fd_ = -1;
};

enum class ReceiveStatus {
  Ok,
  Timeout,   // Nothing arrived in time, or a signal interrupted the wait.
  Truncated, // The datagram was larger than the buffer; its data is invalid.
};

struct ReceiveResult {
  ReceiveStatus status;
  std::size_t size; // Bytes written to the buffer; valid only for Ok.
};

const char *to_string(ReceiveStatus status);

// Receives datagrams on a local port. Not thread-safe.
class UdpReceiver {
public:
  // Binds to `address` ("0.0.0.0" for all IPv4 interfaces). Port 0 picks a
  // free port; see port(). Throws like UdpSender.
  explicit UdpReceiver(std::uint16_t port,
                       const std::string &address = "0.0.0.0");
  ~UdpReceiver();

  UdpReceiver(UdpReceiver &&other) noexcept;
  UdpReceiver &operator=(UdpReceiver &&other) noexcept;
  UdpReceiver(const UdpReceiver &) = delete;
  UdpReceiver &operator=(const UdpReceiver &) = delete;

  // The bound local port.
  std::uint16_t port() const;

  // Waits up to `timeout` for one datagram and copies it into `buffer`.
  // Returns Timeout on signals too, so callers can check for shutdown.
  // Throws std::system_error on socket errors.
  ReceiveResult receive(void *buffer, std::size_t capacity,
                        std::chrono::milliseconds timeout);

private:
  int fd_ = -1;
};

} // namespace comm
