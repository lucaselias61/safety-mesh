#include "worker.hpp"
#include "pipeline.hpp"
#include "camera.hpp"
#include "shutdown.hpp"

#include <iostream>


void Worker::run() {
    pipeline_.start();
    running_ = true;

    try {
        while (running_ && !shutdown_requested) {
            auto frames = pipeline_.get_frames();
            auto detections = detector.infer(frames.get_color_frame());
        }
    } catch (const std::exception& e) {
        std::cerr << "(Worker Exception): " << e.what() << '\n';
        std::cerr << "Stopping worker...\n";
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

