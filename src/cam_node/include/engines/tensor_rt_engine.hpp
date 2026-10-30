#pragma once

#include "engine.hpp"
#include "utils.hpp"

#include <memory>

// TensorRT 10+ runtime for a trusted, prebuilt YOLOv8/YOLO11 engine.
// Expects FP32 linear tensors: [1, 3, input_size, input_size] -> [1, 84, N].
// Each instance owns its execution context; detect must not run concurrently.
class TensorRTEngine : public Engine {
public:
  explicit TensorRTEngine(const EngineConfig &config);
  ~TensorRTEngine() override;

  TensorRTEngine(const TensorRTEngine &) = delete;
  TensorRTEngine &operator=(const TensorRTEngine &) = delete;

  std::vector<Detection> detect(const rs2::video_frame &frame) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
