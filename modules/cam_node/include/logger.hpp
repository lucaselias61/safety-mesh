#pragma once

#include "engines/engine.hpp"
#include "geometry.hpp"
#include "pipeline.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// Durations of one Worker loop iteration.
struct LoopTimings {
  std::chrono::steady_clock::duration wait;   // Waiting for the frameset.
  std::chrono::steady_clock::duration detect; // Engine inference.
};

/// Least to most severe.
enum class LogLevel { debug, info, warn, error };

/// @return The matching level, or std::nullopt if unknown.
std::optional<LogLevel> parse_log_level(std::string_view text);

class ViewerSink;

/// @brief cam_node's single logging front end: decides what goes where.
/// @details Text at or above the level: debug/info to stdout, warn/error to
/// stderr. Frames and calibration also go to Rerun if set up. Not thread-safe.
class Logger {
public:
  /// Text only, no viewer.
  explicit Logger(LogLevel level = LogLevel::info);

  /// @brief Reads LOG_LEVEL (default info), RERUN_URL and RERUN_IMAGE_FPS.
  /// @details Bad values warn and fall back. The viewer never stops the camera.
  static Logger from_env(const std::string &serial);

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

  /// Logs camera models. Call after each pipeline start.
  void calibration(const geometry::Calibration &calibration);

  /// Logs a frameset with its detections and timings.
  void frame(const Frames &frames, const std::vector<Detection> &detections,
             const LoopTimings &timings);

private:
  void write(LogLevel level, std::string_view message) const;

  /// Defined in logger.cpp, where ViewerSink is complete.
  struct ViewerDeleter {
    void operator()(ViewerSink *viewer) const;
  };

  LogLevel level_;
  std::unique_ptr<ViewerSink, ViewerDeleter> viewer_; // nullptr: no viewer.
};
