#pragma once

#include "engines/engine_helpers.hpp"

#include <string>

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
