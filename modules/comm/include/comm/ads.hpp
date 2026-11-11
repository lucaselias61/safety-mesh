#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

// Reads PLC variables as raw bytes from a Beckhoff TwinCAT runtime over ADS.
// Requires a static AMS route for this machine's AMS Net ID and IP,
namespace comm {

// An ADS request failed. code() is the ADS return code (see Beckhoff's
// "ADS Return Codes"), e.g. 0x710 symbol not found, 0x745 timeout.
class AdsError : public std::runtime_error {
public:
  AdsError(const std::string &what, long code);
  long code() const noexcept { return code_; }

private:
  long code_;
};

struct AdsConfig {
  std::string plc_ip;       // ipv4 address
  std::string plc_net_id;   // AMS Net ID of the PLC, e.g. "<ip>.1.1"
  std::uint16_t plc_port;   // 851 = PLC runtime 1, 852 = PLC runtime 2, ...
  std::string local_net_id; // this machine's AMS Net ID
  std::chrono::milliseconds timeout{100};
};

// One connection to one PLC runtime. Symbol handles are cached and released
// automatically. Not thread-safe.
//
// ADS library processes local AMS Net ID, so all clients in
// one process must use the same local_net_id.
class AdsClient {
public:
  explicit AdsClient(const AdsConfig &config);
  ~AdsClient();

  AdsClient(AdsClient &&) noexcept;
  AdsClient &operator=(AdsClient &&) noexcept;
  AdsClient(const AdsClient &) = delete;
  AdsClient &operator=(const AdsClient &) = delete;

  // Size in bytes of symbol (GVL variable).
  std::size_t symbol_size(const std::string &symbol);

  // Reads all of GVL variable in one request. The cached handle is dropped so
  // the next call re-resolves it.
  void read(const std::string &symbol, void *buffer, std::size_t size);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace comm
