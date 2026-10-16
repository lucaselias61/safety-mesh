#pragma once

#include "camera.hpp"
#include "pipeline.hpp"
#include "vision.hpp"

class Worker {
public:
    Worker(const Camera& cam, const PipelineProfile& profile)
        : cam_(cam), pipeline_(cam, profile), engine_(pipeline_) {}

    void run();
    void stop();
    bool is_running() const;

private:
    Camera cam_;
    bool running_ = false;
    Pipeline pipeline_;
    Inference engine_;
};
