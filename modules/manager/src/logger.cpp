#include "logger.hpp"

#ifdef MANAGER_WITH_RERUN
#include "viewer_sink.hpp"
#endif

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

/// @return The value of the environment variable, or "" if unset.
std::string env(const char *name) {
  const char *value = std::getenv(name);
  return value ? value : "";
}

} // namespace

std::optional<LogLevel> parse_log_level(std::string_view text) {
  if (text == "debug")
    return LogLevel::debug;
  if (text == "info")
    return LogLevel::info;
  if (text == "warn")
    return LogLevel::warn;
  if (text == "error")
    return LogLevel::error;
  return std::nullopt;
}

void Logger::ViewerDeleter::operator()(
    [[maybe_unused]] ViewerSink *viewer) const {
#ifdef MANAGER_WITH_RERUN
  delete viewer;
#endif
  // Without Rerun, no ViewerSink is ever created.
}

Logger::Logger(LogLevel level) : level_(level) {}
Logger::Logger(Logger &&) noexcept = default;
Logger &Logger::operator=(Logger &&) noexcept = default;
Logger::~Logger() = default;

Logger Logger::from_env() {
  Logger logger;
  const std::string level_text = env("LOG_LEVEL");
  if (!level_text.empty()) {
    if (const auto level = parse_log_level(level_text))
      logger.level_ = *level;
    else
      logger.warn("LOG_LEVEL must be debug, info, warn or error, got '" +
                  level_text + "'; using info");
  }

#ifdef MANAGER_WITH_RERUN
  try {
    if (const auto settings = rerun_logger::settings_from_env()) {
      // Empty root: the manager logs cams/<serial> and robots/<id>.
      auto connection = rerun_logger::ViewerConnection::connect(*settings, "");
      logger.info("Logging to Rerun at " + connection->url());
      logger.viewer_.reset(new ViewerSink(std::move(connection)));
    }
  } catch (const std::exception &e) {
    logger.warn(std::string("Rerun: ") + e.what() + "; not logging");
  }
#else
  if (!env("RERUN_URL").empty())
    logger.warn("RERUN_URL is set, but the manager was built without Rerun "
                "(-DMANAGER_RERUN=OFF); not logging");
#endif
  return logger;
}

void Logger::write(LogLevel level, std::string_view message) const {
  if (!enabled(level))
    return;
  auto &out = level >= LogLevel::warn ? std::cerr : std::cout;
  out << message << '\n';
}

void Logger::cameras(const std::vector<CameraConfig> &cameras) {
#ifdef MANAGER_WITH_RERUN
  if (viewer_)
    viewer_->cameras(cameras);
#endif
  for (const auto &camera : cameras) {
    const auto &t = camera.extrinsics.camera_to_world;
    info(fmt::format("Camera {} ({}) at [{:.3f} {:.3f} {:.3f}] m, "
                     "reprojection {:.3f} px",
                     camera.id, camera.extrinsics.serial, t[0][3], t[1][3],
                     t[2][3], camera.extrinsics.reprojection_error_px));
  }
}

void Logger::robot(const RobotState &state) {
#ifdef MANAGER_WITH_RERUN
  if (viewer_)
    viewer_->robot(state);
#endif
  if (!enabled(LogLevel::debug))
    return;
  debug(fmt::format("Robot {} cycle {} tcp [{:.3f}] m{}", state.id, state.cycle,
                    fmt::join(state.tcp, " "), state.stale ? " STALE" : ""));
}
