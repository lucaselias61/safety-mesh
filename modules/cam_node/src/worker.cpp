#include "worker.hpp"
#include "camera.hpp"
#include "pipeline.hpp"
#include "shutdown.hpp"

#include <iostream>
#include <memory>
#include <utility>

void Worker::run() {
  pipeline_.start();
  running_ = true;
  int retry_count = 0;

  try {
    while (running_ && !shutdown_requested) {
      auto frames = pipeline_.get_frames();
      if (!frames) {
        std::cerr << "No frames received, retrying...\n";
        if (++retry_count >= 5) {
          std::cerr << "Max retry count reached\n";
          break;
        }
        continue;
      }
      std::vector<Detection> detections =
          engine_->detect(frames.get_color_frame());
      retry_count = 0; // Reset retry count on successful detection
      for (const auto &detection : detections) {
        std::cout << "Detected object with confidence: " << detection.confidence
                  << "\n";
        std::cout << "Bounding box [x, y, width, height]: [" << detection.bbox.x
                  << ", " << detection.bbox.y << ", " << detection.bbox.width
                  << ", " << detection.bbox.height << "]\n";
      }
    }
  } catch (...) {
    std::cerr << "Stopping worker...\n";
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
