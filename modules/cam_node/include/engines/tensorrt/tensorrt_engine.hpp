#pragma once

#include "engines/engine.hpp"
#include "utils.hpp"

#include <memory>

// TensorRT 10+ runtime for a YOLO model.
//
// - Loads an ONNX model (built and cached on first use) or a trusted prebuilt
//   plan. See resolve_tensorrt_plan.
// - In-/output is FP32 linear for every engine.precision.
// - [1, 3, input_size, input_size] -> [1, 84, N] detection or [1, 56, N] pose.
// - Each instance owns its execution context. detect must not run concurrently.
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
