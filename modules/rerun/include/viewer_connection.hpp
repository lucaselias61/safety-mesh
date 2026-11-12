#pragma once

#include <rerun.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace rerun_logger {

/// Viewer URL and image rate.
struct ViewerSettings {
  std::string url;        // e.g. "rerun+http://rerun:9876/proxy"
  double image_fps = 5.0; // Maximum image rate; 0 sends no images.
};

/// @brief Reads RERUN_URL and RERUN_IMAGE_FPS (default 5).
/// @return std::nullopt if RERUN_URL is empty (viewer off).
/// @throws std::invalid_argument if RERUN_IMAGE_FPS isn't a number >= 0.
std::optional<ViewerSettings> settings_from_env();

/// @brief A node's connection to the shared Rerun viewer. Debug only: not on
/// the safety path.
/// @details All nodes share one recording, so one timeline. Paths are
/// relative to the node's root, e.g. "cams/<serial>". Not thread-safe.
class ViewerConnection {
public:
  /// @brief Non-blocking; the SDK connects and reconnects in the background.
  /// @param root Path prefix for this node.
  /// @throws std::invalid_argument if the URL is invalid.
  static std::unique_ptr<ViewerConnection>
  connect(const ViewerSettings &settings, std::string root);

  const std::string &root() const { return root_; }
  const std::string &url() const { return url_; }

  /// @return root/child, or root if child is empty.
  std::string path(std::string_view child) const;

  /// Logs under root/child at the current time.
  template <typename... Archetypes>
  void log(std::string_view child, const Archetypes &...archetypes) {
    rec_.log(path(child), archetypes...);
  }

  /// Logs under root/child for all time (e.g. calibration).
  template <typename... Archetypes>
  void log_static(std::string_view child, const Archetypes &...archetypes) {
    rec_.log_static(path(child), archetypes...);
  }

  /// Sets the "frame" and "capture_time" timelines for following logs.
  void set_time(std::int64_t frame, std::int64_t capture_time_ns);

  /// @return True at most image_fps times per second; never if 0.
  bool image_due();

private:
  using Clock = std::chrono::steady_clock;

  ViewerConnection(std::string root, std::string url, double image_fps);

  rerun::RecordingStream rec_;
  std::string root_;
  std::string url_;
  bool images_enabled_;
  Clock::duration image_period_;
  Clock::time_point last_image_{};
};

} // namespace rerun_logger
