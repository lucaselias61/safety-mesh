#pragma once

#include "camera.hpp"
#include "engines/engine.hpp"
#include "logger.hpp"
#include "pipeline.hpp"

#include <memory>

class Worker {
public:
  /// @param logger Text and viewer output.
  explicit Worker(const Camera &cam, const PipelineProfile &profile,
                  std::unique_ptr<Engine> engine, Logger logger = Logger())
      : cam_(cam), pipeline_(cam, profile), engine_(std::move(engine)),
        logger_(std::move(logger)) {}

  void run();
  void stop();
  bool is_running() const;

private:
  Camera cam_;
  bool running_ = false;
  Pipeline pipeline_;
  std::unique_ptr<Engine> engine_;
  Logger logger_;
};
