#pragma once

#include <string>

// Returns the path of a TensorRT plan for the current GPU.
//
// Paths that do not end in .onnx are treated as prebuilt plans and returned
// unchanged. An ONNX model is built once with a [1, 3, input_size, input_size]
// FP32 input and cached as <model dir>/.engines/<key>.engine. The key covers
// the model contents, GPU, driver, CUDA, TensorRT and input size, so a change
// to any of them triggers a rebuild. Processes sharing the directory build each
// plan once. Building can take several minutes.
//
// Throws std::runtime_error if the build fails or shutdown is requested.
std::string resolve_tensorrt_plan(const std::string &model_path,
                                  int input_size);
