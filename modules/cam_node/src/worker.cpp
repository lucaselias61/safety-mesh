#include "worker.hpp"
#include "camera.hpp"
#include "frames.hpp"
#include "pipeline.hpp"
#include "shutdown.hpp"

#include <chrono>
#include <memory>
#include <utility>

void Worker::run() {
  pipeline_.start();
  running_ = true;
  if (pipeline_.calibration())
    logger_.calibration(*pipeline_.calibration());
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
      std::vector<Detection> detections = engine_->detect(color);
      const auto detected = std::chrono::steady_clock::now();
      const LoopTimings timings{frames_ready - wait_start,
                                detected - frames_ready, log_time,
                                frame_age(color)};
      logger_.frame(frames, detections, timings);
      log_time = std::chrono::steady_clock::now() - detected;
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
