#pragma once

#include "camera.hpp"
#include "pipeline.hpp"
#include "vision.hpp"
#include "pipeline.hpp"

class Worker {
public:
    explicit Worker(const Camera& cam) : cam_(cam), pipeline_(cam), engine_(pipeline_) {}

    void run();
    void stop();
    bool is_running() const;

private:
    Camera cam_;
    bool running_ = false;
    Pipeline pipeline_;
    Inference engine_;
};
