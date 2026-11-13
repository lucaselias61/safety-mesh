#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

// Precision the engine computes in. In-/output stays FP32.
enum class Precision : std::uint8_t { fp32, fp16, int8 };

inline Precision parse_precision(std::string_view name) {
  if (name == "fp32")
    return Precision::fp32;
  if (name == "fp16")
    return Precision::fp16;
  if (name == "int8")
    return Precision::int8;
  throw std::invalid_argument("precision must be fp32, fp16 or int8, not '" +
                              std::string(name) + "'");
}

inline const char *to_string(Precision precision) {
  switch (precision) {
  case Precision::fp32:
    return "fp32";
  case Precision::fp16:
    return "fp16";
  case Precision::int8:
    return "int8";
  }
  return "unknown";
}

// Returns the path of a TensorRT plan for the current GPU.
//
// - Paths !=".onnx" are treated as prebuilt plans and returned. precision does
// not apply.
// - An ONNX model is built once with a [1, 3, input_size, input_size] FP32
// in-/output, in the given precision.
// - Cached as <model dir>/.engines/<key>.engine. Key covers GPU, driver, CUDA,
// TensorRT, input size and precision if changed rebuild is triggered.
// - int8 needs an ONNX model with Q/DQ (quantize/dequantize) nodes. Calibration
// is not supported.
// - Processes sharing the directory build plan once.
//
// Throws std::runtime_error if build fails or shutdown requested.
std::string resolve_tensorrt_plan(const std::string &model_path, int input_size,
                                  Precision precision);
