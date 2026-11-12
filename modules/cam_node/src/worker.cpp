#include "worker.hpp"
#include "camera.hpp"
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
      std::vector<Detection> detections =
          engine_->detect(frames.get_color_frame());
      logger_.frame(frames, detections,
                    {frames_ready - wait_start,
                     std::chrono::steady_clock::now() - frames_ready});
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
