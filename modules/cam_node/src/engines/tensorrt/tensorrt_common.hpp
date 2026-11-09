#pragma once

// Private helpers shared by the TensorRT engine and builder.

#include <NvInfer.h>
#include <cuda_runtime_api.h>

#include <cstdio>
#include <stdexcept>
#include <string>

#if NV_TENSORRT_MAJOR < 10
#error "The TensorRT backend requires TensorRT 10 or newer"
#endif

namespace tensorrt {

inline void handle_cuda_error(cudaError_t result) {
  if (result != cudaSuccess)
    throw std::runtime_error(std::string("(CUDA error): ") +
                             cudaGetErrorString(result));
}

struct Logger : nvinfer1::ILogger {
  void log(Severity severity, const char *message) noexcept override {
    if (severity <= Severity::kWARNING)
      std::fprintf(stderr, "TensorRT: %s\n", message);
  }
};

} // namespace tensorrt
