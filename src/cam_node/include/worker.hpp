#pragma once

#include "camera.hpp"
#include "pipeline.hpp"
#include "engines/engine.hpp"


class Worker {
public:
    explicit Worker(const Camera& cam, const PipelineProfile& profile, std::unique_ptr<Engine> engine)
        : cam_(cam), pipeline_(cam, profile), engine_(std::move(engine)) {}

    void run();
    void stop();
    bool is_running() const;

private:
    Camera cam_;
    bool running_ = false;
    Pipeline pipeline_;
    std::unique_ptr<Engine> engine_;
};
