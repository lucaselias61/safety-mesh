#include <comm/ads.hpp>

#include <AdsDevice.h>
#include <AdsLib.h>

#include <cstdio>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace comm {
namespace {

// AdsSymbolEntry header plus symbol name, type name and comment strings.
constexpr std::size_t kSymbolInfoCapacity = 64 * 1024;

std::string hex(long code) {
  char text[16];
  std::snprintf(text, sizeof(text), "0x%lX", code);
  return text;
}

AmsNetId parse_net_id(const std::string &text) {
  const AmsNetId id{text};
  if (!id)
    throw std::invalid_argument("Invalid AMS Net ID: " + text);
  return id;
}

struct Symbol {
  AdsHandle handle;
  std::size_t size;
};

} // namespace

AdsError::AdsError(const std::string &what, long code)
    : std::runtime_error(what + " (ADS error " + hex(code) + ")"), code_(code) {
}

struct AdsClient::Impl {
  AdsDevice device;
  std::unordered_map<std::string, Symbol> symbols;

  explicit Impl(const AdsConfig &config)
      : device(config.plc_ip, parse_net_id(config.plc_net_id),
               config.plc_port) {
    device.SetTimeout(static_cast<uint32_t>(config.timeout.count()));
  }

  std::size_t query_size(const std::string &name) const {
    std::vector<std::uint8_t> info(kSymbolInfoCapacity);
    uint32_t bytes_read = 0;
    const long error = device.ReadWriteReqEx2(
        ADSIGRP_SYM_INFOBYNAMEEX, 0, info.size(), info.data(), name.size(),
        name.c_str(), &bytes_read);
    if (error)
      throw AdsError("Cannot read symbol info for " + name, error);
    if (bytes_read < sizeof(AdsSymbolEntry))
      throw AdsError("Short symbol info for " + name,
                     ADSERR_DEVICE_INVALIDSIZE);
    AdsSymbolEntry entry;
    std::memcpy(&entry, info.data(), sizeof(entry));
    return entry.size;
  }

  Symbol &resolve(const std::string &name) {
    if (auto it = symbols.find(name); it != symbols.end())
      return it->second;
    const std::size_t size = query_size(name);
    AdsHandle handle = [&] {
      try {
        return device.GetHandle(name);
      } catch (const AdsException &error) {
        throw AdsError("Cannot get handle for " + name, error.errorCode);
      }
    }();
    return symbols.emplace(name, Symbol{std::move(handle), size}).first->second;
  }
};

AdsClient::AdsClient(const AdsConfig &config) {
  bhf::ads::SetLocalAddress(parse_net_id(config.local_net_id));
  try {
    impl_ = std::make_unique<Impl>(config);
  } catch (const AdsException &error) {
    throw AdsError("Cannot open ADS route to " + config.plc_ip,
                   error.errorCode);
  }
}

AdsClient::~AdsClient() = default;
AdsClient::AdsClient(AdsClient &&) noexcept = default;
AdsClient &AdsClient::operator=(AdsClient &&) noexcept = default;

std::size_t AdsClient::symbol_size(const std::string &symbol) {
  return impl_->resolve(symbol).size;
}

void AdsClient::read(const std::string &symbol, void *buffer,
                     std::size_t size) {
  const Symbol &entry = impl_->resolve(symbol);
  if (size != entry.size)
    throw AdsError("Size mismatch for " + symbol + ": PLC has " +
                       std::to_string(entry.size) + " bytes, caller expects " +
                       std::to_string(size),
                   ADSERR_DEVICE_INVALIDSIZE);
  if (size > std::numeric_limits<uint32_t>::max())
    throw AdsError("Symbol too large: " + symbol, ADSERR_DEVICE_INVALIDSIZE);

  uint32_t bytes_read = 0;
  const long error = impl_->device.ReadReqEx2(
      ADSIGRP_SYM_VALBYHND, *entry.handle, size, buffer, &bytes_read);
  if (error || bytes_read != size) {
    impl_->symbols.erase(symbol); // Re-resolve after restarts/online changes.
    throw AdsError("Cannot read " + symbol,
                   error ? error : ADSERR_DEVICE_INVALIDSIZE);
  }
}

} // namespace comm
