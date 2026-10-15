#include "worker.hpp"
#include "pipeline.hpp"
#include "camera.hpp"
#include "shutdown.hpp"
#include "vision.hpp"

#include <iostream>


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
                    break;
                }
                continue;
            }
            // auto detections = detector_.detect(frames.get_color_frame());
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

bool Worker::is_running() const {
    return running_;
}

