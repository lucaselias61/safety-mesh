#pragma once

#include "config.hpp"
#include "transform.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// One robot poll, in the world frame, meters.
struct RobotState {
  std::string id;
  std::uint32_t cycle; // PLC write counter.
  /// Base, then the end of each link. std::nullopt without DH values.
  std::optional<std::vector<Point3>> joints;
  Point3 tcp;
  bool stale; // PLC cycle stopped advancing.
};

/// Least to most severe.
enum class LogLevel : std::uint8_t { debug, info, warn, error };

/// @return The matching level, or std::nullopt if unknown.
std::optional<LogLevel> parse_log_level(std::string_view text);

class ViewerSink;

/// @brief The manager's single logging front end: decides what goes where.
/// @details Text at or above the level: debug/info to stdout, warn/error to
/// stderr. Camera poses and robots also go to Rerun if set up. Not
/// thread-safe.
class Logger {
public:
  /// Text only, no viewer.
  explicit Logger(LogLevel level = LogLevel::info);

  /// @brief Reads LOG_LEVEL (default info) and RERUN_URL.
  /// @details Bad values warn and fall back. The viewer never stops the
  /// manager.
  static Logger from_env();

  Logger(Logger &&) noexcept;
  Logger &operator=(Logger &&) noexcept;
  ~Logger();

  bool enabled(LogLevel level) const { return level >= level_; }

  void debug(std::string_view message) const {
    write(LogLevel::debug, message);
  }
  void info(std::string_view message) const { write(LogLevel::info, message); }
  void warn(std::string_view message) const { write(LogLevel::warn, message); }
  void error(std::string_view message) const {
    write(LogLevel::error, message);
  }

  /// Logs each camera's pose in the world. Call once after loading.
  void cameras(const std::vector<CameraConfig> &cameras);

  /// Logs one robot poll.
  void robot(const RobotState &state);

private:
  void write(LogLevel level, std::string_view message) const;

  /// Defined in logger.cpp, where ViewerSink is complete.
  struct ViewerDeleter {
    void operator()(ViewerSink *viewer) const;
  };

  LogLevel level_;
  std::unique_ptr<ViewerSink, ViewerDeleter> viewer_; // nullptr: no viewer.
};
