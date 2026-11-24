#include "worker.hpp"
#include "camera.hpp"
#include "frames.hpp"
#include "geometry.hpp"
#include "pipeline.hpp"
#include "shutdown.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

/// @return 3D poses of the detections. Empty if the frameset has no depth.
/// @throws std::invalid_argument if the depth frame is not Z16.
std::vector<geometry::Detection3D>
poses_in_3d(const Frames &frames, const std::vector<Detection2D> &detections,
            const geometry::Calibration &calibration) {
  std::vector<geometry::Detection3D> poses;
  const auto depth = frames.get_depth_frame();
  if (!depth)
    return poses;
  const auto timestamp = capture_time(frames.get_color_frame());
  poses.reserve(detections.size());
  for (const auto &detection : detections)
    poses.push_back(
        geometry::detection_in_3d(detection, depth, calibration, timestamp));
  return poses;
}

} // namespace

void Worker::run() {
  pipeline_.start();
  running_ = true;
  // Color and depth always stream, so the pipeline has their calibration.
  if (!pipeline_.calibration()) {
    stop();
    throw std::runtime_error(
        "No color and depth calibration. Both must stream.");
  }
  // A copy: stop() clears the pipeline's.
  const geometry::Calibration calibration = *pipeline_.calibration();
  logger_.calibration(calibration);
  int retry_count = 0;
  // logger_.frame() can't time itself, so each frame reports the previous one.
  auto log_time = std::chrono::steady_clock::duration::zero();

  try {
    while (running_ && !shutdown_requested) {
      const auto wait_start = std::chrono::steady_clock::now();
      auto frames = pipeline_.get_frames();
      const auto frames_ready = std::chrono::steady_clock::now();
      if (!frames) {
        logger_.warn("No frames received, retrying...");
        if (++retry_count >= 5) {
          logger_.error("Max retry count reached");
          break;
        }
        continue;
      }
      const auto color = frames.get_color_frame();
      std::vector<Detection2D> detections = engine_->detect(color);
      const auto detected = std::chrono::steady_clock::now();
      const auto poses = poses_in_3d(frames, detections, calibration);
      const auto deprojected = std::chrono::steady_clock::now();
      const LoopTimings timings{frames_ready - wait_start,
                                detected - frames_ready, deprojected - detected,
                                log_time, frame_age(color)};
      logger_.frame(frames, detections, poses, timings);
      log_time = std::chrono::steady_clock::now() - deprojected;
      retry_count = 0; // Reset retry count on successful detection
    }
  } catch (...) {
    logger_.error("Stopping worker...");
    stop();
    throw;
  }
  stop();
}

void Worker::stop() {
  if (!running_) {
    return;
  }
  pipeline_.stop();
  running_ = false;
}

bool Worker::is_running() const { return running_; }
