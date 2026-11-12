#include "viewer_connection.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace rerun_logger {

namespace {

/// @return The value of the environment variable, or "" if unset.
std::string env(const char *name) {
  const char *value = std::getenv(name);
  return value ? value : "";
}

} // namespace

std::optional<ViewerSettings> settings_from_env() {
  ViewerSettings settings;
  settings.url = env("RERUN_URL");
  if (settings.url.empty())
    return std::nullopt;

  const std::string fps_text = env("RERUN_IMAGE_FPS");
  if (!fps_text.empty()) {
    char *end = nullptr;
    const double fps = std::strtod(fps_text.c_str(), &end);
    if (end != fps_text.c_str() + fps_text.size() || !(fps >= 0.0) ||
        !std::isfinite(fps))
      throw std::invalid_argument(
          "RERUN_IMAGE_FPS must be a number >= 0, got '" + fps_text + "'");
    settings.image_fps = fps;
  }
  return settings;
}

ViewerConnection::ViewerConnection(std::string root, std::string url,
                                   double image_fps)
    // One recording shared by all nodes: one timeline.
    : rec_("safety_mesh", "safety_mesh"), root_(std::move(root)),
      url_(std::move(url)), images_enabled_(image_fps > 0.0),
      image_period_(images_enabled_
                        ? std::chrono::duration_cast<Clock::duration>(
                              std::chrono::duration<double>(1.0 / image_fps))
                        : Clock::duration::zero()) {}

std::unique_ptr<ViewerConnection>
ViewerConnection::connect(const ViewerSettings &settings, std::string root) {
  // Private constructor, so no std::make_unique.
  std::unique_ptr<ViewerConnection> connection(
      new ViewerConnection(std::move(root), settings.url, settings.image_fps));
  const auto error = connection->rec_.connect_grpc(settings.url);
  if (error.is_err())
    throw std::invalid_argument("cannot use RERUN_URL '" + settings.url +
                                "': " + error.description);
  return connection;
}

std::string ViewerConnection::path(std::string_view child) const {
  if (child.empty())
    return root_;
  std::string result = root_;
  result += '/';
  result += child;
  return result;
}

void ViewerConnection::set_time(std::int64_t frame,
                                std::int64_t capture_time_ns) {
  rec_.set_time_sequence("frame", frame);
  rec_.set_time_timestamp_nanos_since_epoch("capture_time", capture_time_ns);
}

bool ViewerConnection::image_due() {
  if (!images_enabled_)
    return false;
  const auto now = Clock::now();
  if (now - last_image_ < image_period_)
    return false;
  last_image_ = now;
  return true;
}

} // namespace rerun_logger
