#include <comm/transport/udp.hpp>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace comm::transport {
namespace {

using AddressList = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>;

AddressList resolve(const std::string &host, std::uint16_t port, bool passive) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_NUMERICSERV | (passive ? AI_PASSIVE : 0);
  const std::string service = std::to_string(port);
  addrinfo *result = nullptr;
  const int error = getaddrinfo(host.empty() ? nullptr : host.c_str(),
                                service.c_str(), &hints, &result);
  if (error != 0)
    throw std::runtime_error("Cannot resolve " + host + ":" + service + ": " +
                             gai_strerror(error));
  return {result, &freeaddrinfo};
}

// Opens a UDP socket and binds it (receiver) or connects it (sender) to the
// first usable address. Connecting fixes the destination, so send() needs no
// address and the OS can report ICMP errors back to the sender.
int open_socket(const std::string &host, std::uint16_t port, bool passive) {
  const AddressList addresses = resolve(host, port, passive);
  int error = EADDRNOTAVAIL;
  for (const addrinfo *address = addresses.get(); address != nullptr;
       address = address->ai_next) {
    const int fd = ::socket(address->ai_family, address->ai_socktype,
                            address->ai_protocol);
    if (fd < 0) {
      error = errno;
      continue;
    }
    // Do not leak the socket into child processes.
    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    const int result =
        passive ? ::bind(fd, address->ai_addr, address->ai_addrlen)
                : ::connect(fd, address->ai_addr, address->ai_addrlen);
    if (result == 0)
      return fd;
    error = errno;
    ::close(fd);
  }
  throw std::system_error(
      error, std::generic_category(),
      std::string(passive ? "Cannot bind " : "Cannot connect ") + host + ":" +
          std::to_string(port));
}

void close_socket(int &fd) {
  if (fd >= 0)
    ::close(fd);
  fd = -1;
}

} // namespace

UdpSender::UdpSender(const std::string &host, std::uint16_t port)
    : fd_(open_socket(host, port, false)) {}

UdpSender::~UdpSender() { close_socket(fd_); }

UdpSender::UdpSender(UdpSender &&other) noexcept
    : fd_(std::exchange(other.fd_, -1)) {}

UdpSender &UdpSender::operator=(UdpSender &&other) noexcept {
  if (this != &other) {
    close_socket(fd_);
    fd_ = std::exchange(other.fd_, -1);
  }
  return *this;
}

bool UdpSender::send(const void *data, std::size_t size) {
  ssize_t sent = 0;
  do {
    sent = ::send(fd_, data, size, 0);
  } while (sent < 0 && errno == EINTR);
  return sent >= 0 && static_cast<std::size_t>(sent) == size;
}

const char *to_string(ReceiveStatus status) {
  switch (status) {
  case ReceiveStatus::Ok:
    return "ok";
  case ReceiveStatus::Timeout:
    return "timeout";
  case ReceiveStatus::Truncated:
    return "truncated";
  }
  return "unknown";
}

UdpReceiver::UdpReceiver(std::uint16_t port, const std::string &address)
    : fd_(open_socket(address, port, true)) {}

UdpReceiver::~UdpReceiver() { close_socket(fd_); }

UdpReceiver::UdpReceiver(UdpReceiver &&other) noexcept
    : fd_(std::exchange(other.fd_, -1)) {}

UdpReceiver &UdpReceiver::operator=(UdpReceiver &&other) noexcept {
  if (this != &other) {
    close_socket(fd_);
    fd_ = std::exchange(other.fd_, -1);
  }
  return *this;
}

std::uint16_t UdpReceiver::port() const {
  sockaddr_storage address{};
  socklen_t length = sizeof(address);
  if (::getsockname(fd_, reinterpret_cast<sockaddr *>(&address), &length) != 0)
    throw std::system_error(errno, std::generic_category(),
                            "Cannot read bound port");
  if (address.ss_family == AF_INET)
    return ntohs(reinterpret_cast<const sockaddr_in &>(address).sin_port);
  if (address.ss_family == AF_INET6)
    return ntohs(reinterpret_cast<const sockaddr_in6 &>(address).sin6_port);
  throw std::runtime_error("Unexpected socket address family");
}

ReceiveResult UdpReceiver::receive(void *buffer, std::size_t capacity,
                                   std::chrono::milliseconds timeout) {
  pollfd ready{fd_, POLLIN, 0};
  const auto wait =
      std::clamp<std::chrono::milliseconds::rep>(timeout.count(), 0, INT_MAX);
  const int events = ::poll(&ready, 1, static_cast<int>(wait));
  if (events < 0) {
    if (errno == EINTR)
      return {ReceiveStatus::Timeout, 0};
    throw std::system_error(errno, std::generic_category(), "poll failed");
  }
  if (events == 0)
    return {ReceiveStatus::Timeout, 0};

  // recvmsg reports MSG_TRUNC portably when the datagram did not fit.
  iovec segment{buffer, capacity};
  msghdr message{};
  message.msg_iov = &segment;
  message.msg_iovlen = 1;
  const ssize_t received = ::recvmsg(fd_, &message, 0);
  if (received < 0) {
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
      return {ReceiveStatus::Timeout, 0};
    throw std::system_error(errno, std::generic_category(), "recvmsg failed");
  }
  if ((message.msg_flags & MSG_TRUNC) != 0)
    return {ReceiveStatus::Truncated, 0};
  return {ReceiveStatus::Ok, static_cast<std::size_t>(received)};
}

} // namespace comm::transport
