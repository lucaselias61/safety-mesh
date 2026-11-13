#include "logger.hpp"

#ifdef CAM_NODE_WITH_RERUN
#include "viewer_sink.hpp"
#endif

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
#ifdef CAM_NODE_WITH_RERUN
  delete viewer;
#endif
  // Without Rerun, no ViewerSink is ever created.
}

Logger::Logger(LogLevel level) : level_(level) {}
Logger::Logger(Logger &&) noexcept = default;
Logger &Logger::operator=(Logger &&) noexcept = default;
Logger::~Logger() = default;

Logger Logger::from_env([[maybe_unused]] const std::string &serial) {
  Logger logger;
  const std::string level_text = env("LOG_LEVEL");
  if (!level_text.empty()) {
    if (const auto level = parse_log_level(level_text))
      logger.level_ = *level;
    else
      logger.warn("LOG_LEVEL must be debug, info, warn or error, got '" +
                  level_text + "'; using info");
  }

#ifdef CAM_NODE_WITH_RERUN
  try {
    if (const auto settings = rerun_logger::settings_from_env()) {
      auto connection =
          rerun_logger::ViewerConnection::connect(*settings, "cams/" + serial);
      logger.info("Logging to Rerun at " + connection->url() + " as " +
                  connection->root());
      // Warnings go to the text log. Capture level, not `this`: the Logger
      // is moved on return.
      const LogLevel level = logger.level_;
      logger.viewer_.reset(new ViewerSink(
          std::move(connection),
          [level](std::string_view message) { Logger(level).warn(message); }));
    }
  } catch (const std::exception &e) {
    logger.warn(std::string("Rerun: ") + e.what() + "; not logging");
  }
#else
  if (!env("RERUN_URL").empty())
    logger.warn("RERUN_URL is set, but cam_node was built without Rerun "
                "(-DCAM_NODE_RERUN=OFF); not logging");
#endif
  return logger;
}

void Logger::write(LogLevel level, std::string_view message) const {
  if (!enabled(level))
    return;
  auto &out = level >= LogLevel::warn ? std::cerr : std::cout;
  out << message << '\n';
}

void Logger::calibration(
    [[maybe_unused]] const geometry::Calibration &calibration) {
#ifdef CAM_NODE_WITH_RERUN
  if (viewer_)
    viewer_->calibration(calibration);
#endif
}

void Logger::frame([[maybe_unused]] const Frames &frames,
                   const std::vector<Detection> &detections,
                   [[maybe_unused]] const LoopTimings &timings) {
#ifdef CAM_NODE_WITH_RERUN
  if (viewer_)
    viewer_->frame(frames, detections, timings);
#endif
  if (!enabled(LogLevel::info))
    return;
  for (const auto &detection : detections) {
    const auto &box = detection.bbox.rect;
    std::cout << "Detected object with confidence: "
              << detection.bbox.confidence << "\n";
    std::cout << "Bounding box [x, y, width, height]: [" << box.x << ", "
              << box.y << ", " << box.width << ", " << box.height << "]\n";
  }
}
