#pragma once

#include "camera.hpp"
#include "pipeline.hpp"

class Worker {
public:
    explicit Worker(const Camera& cam) 
        : cam_(cam), pipeline_(cam) {}

    void run();
    void stop();
    bool is_running() const;

private:
    Camera cam_;
    bool running_ = false;
    Pipeline pipeline_;
};
