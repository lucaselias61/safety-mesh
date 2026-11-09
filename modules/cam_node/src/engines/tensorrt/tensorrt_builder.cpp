#include "engines/tensorrt/tensorrt_builder.hpp"

#include "shutdown.hpp"
#include "tensorrt_common.hpp"

#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <sys/file.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace {
namespace fs = std::filesystem;
using tensorrt::handle_cuda_error;

// Increment when build settings or the cache layout change, so existing plans
// are rebuilt instead of reused.
constexpr int cache_format = 1;

constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

// FNV-1a: a small, stable 64-bit hash. It identifies cache entries; it is not
// a security check, so plans must still come from trusted sources.
std::uint64_t fnv1a(const char *data, std::size_t size,
                    std::uint64_t hash = fnv_offset) {
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= static_cast<unsigned char>(data[i]);
    hash *= fnv_prime;
  }
  return hash;
}

std::string hash_file(const fs::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Cannot open ONNX model: " + path.string());
  std::uint64_t hash = fnv_offset;
  std::vector<char> buffer(1 << 20);
  while (
      file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) ||
      file.gcount() > 0)
    hash = fnv1a(buffer.data(), static_cast<std::size_t>(file.gcount()), hash);
  if (file.bad())
    throw std::runtime_error("Cannot read ONNX model: " + path.string());
  return fmt::format("{:016x}", hash);
}

// Everything that makes a plan incompatible. A change to any value selects a
// different cache entry.
nlohmann::json fingerprint(const fs::path &model, int input_size) {
  int device = 0;
  handle_cuda_error(cudaGetDevice(&device));
  cudaDeviceProp properties{};
  handle_cuda_error(cudaGetDeviceProperties(&properties, device));
  int driver = 0;
  int runtime = 0;
  handle_cuda_error(cudaDriverGetVersion(&driver));
  handle_cuda_error(cudaRuntimeGetVersion(&runtime));
  std::string uuid;
  for (const char byte : properties.uuid.bytes)
    uuid += fmt::format("{:02x}", static_cast<unsigned char>(byte));
  return {
      {"cache_format", cache_format},
      {"onnx_fnv1a64", hash_file(model)},
      {"gpu_name", std::string(properties.name)},
      {"gpu_uuid", uuid},
      {"compute_capability",
       fmt::format("{}.{}", properties.major, properties.minor)},
      {"cuda_driver", driver},
      {"cuda_runtime", runtime},
      {"tensorrt", getInferLibVersion()},
      {"onnx_parser", getNvOnnxParserVersion()},
      {"input", fmt::format("1x3x{}x{}", input_size, input_size)},
      {"precision", "fp32"},
  };
}

// Exclusive lock on a file, released when closed. Processes that share the
// cache directory wait here, so each plan is built once.
class FileLock {
public:
  FileLock(const fs::path &path, const fs::path &plan) {
    fd_ = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd_ < 0)
      throw std::system_error(errno, std::generic_category(),
                              "Cannot open " + path.string());
    bool waiting = false;
    // Poll instead of blocking, so a shutdown request is noticed while another
    // process is building.
    while (::flock(fd_, LOCK_EX | LOCK_NB) != 0) {
      const int error = errno;
      if (error == EINTR)
        continue;
      if (error != EWOULDBLOCK) {
        ::close(fd_);
        throw std::system_error(error, std::generic_category(),
                                "Cannot lock " + path.string());
      }
      if (shutdown_requested) {
        ::close(fd_);
        throw std::runtime_error(
            "Shutdown requested while waiting for TensorRT engine build");
      }
      if (!waiting) {
        std::fprintf(stderr, "Waiting for another process to build %s\n",
                     plan.c_str());
        waiting = true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }
  ~FileLock() { ::close(fd_); }

  FileLock(const FileLock &) = delete;
  FileLock &operator=(const FileLock &) = delete;

private:
  int fd_ = -1;
};

// Logs top-level build phases and cancels the build on SIGINT or SIGTERM.
class BuildProgress : public nvinfer1::IProgressMonitor {
public:
  void phaseStart(const char *phase, const char *parent,
                  int32_t /*steps*/) noexcept override {
    if (parent == nullptr)
      std::fprintf(stderr, "TensorRT build: %s\n", phase);
  }
  bool stepComplete(const char * /*phase*/,
                    int32_t /*step*/) noexcept override {
    return shutdown_requested == 0;
  }
  void phaseFinish(const char * /*phase*/) noexcept override {}
};

void build_plan(const fs::path &model, int input_size, const fs::path &output) {
  // Declaration order matters: objects are destroyed in reverse, so the parser
  // goes before the network, and the logger and monitor outlive everything.
  tensorrt::Logger logger;
  BuildProgress progress;
  std::unique_ptr<nvinfer1::IBuilder> builder{
      nvinfer1::createInferBuilder(logger)};
  if (!builder)
    throw std::runtime_error("Cannot create TensorRT builder");
  std::unique_ptr<nvinfer1::INetworkDefinition> network{
      builder->createNetworkV2(0)};
  if (!network)
    throw std::runtime_error("Cannot create TensorRT network");
  std::unique_ptr<nvonnxparser::IParser> parser{
      nvonnxparser::createParser(*network, logger)};
  if (!parser)
    throw std::runtime_error("Cannot create ONNX parser");
  if (!parser->parseFromFile(
          model.c_str(),
          static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
    std::string message = "Cannot parse ONNX model " + model.string();
    for (int i = 0; i < parser->getNbErrors(); ++i)
      message += std::string("\n  ") + parser->getError(i)->desc();
    throw std::runtime_error(message);
  }

  if (network->getNbInputs() != 1)
    throw std::runtime_error("Expected an ONNX model with one input");
  nvinfer1::ITensor *input = network->getInput(0);
  const nvinfer1::Dims dims = input->getDimensions();
  if (dims.nbDims != 4)
    throw std::runtime_error("Expected a 4D ONNX image input [1, 3, H, W]");
  const nvinfer1::Dims4 shape{1, 3, input_size, input_size};
  std::unique_ptr<nvinfer1::IBuilderConfig> config{
      builder->createBuilderConfig()};
  if (!config)
    throw std::runtime_error("Cannot create TensorRT builder config");
  const bool dynamic =
      std::any_of(dims.d, dims.d + dims.nbDims, [](auto d) { return d < 0; });
  if (dynamic) {
    // Fix dynamic dimensions to the configured size; the engine sets the same
    // shape at runtime.
    nvinfer1::IOptimizationProfile *profile =
        builder->createOptimizationProfile();
    for (const auto selector : {nvinfer1::OptProfileSelector::kMIN,
                                nvinfer1::OptProfileSelector::kOPT,
                                nvinfer1::OptProfileSelector::kMAX})
      if (!profile->setDimensions(input->getName(), selector, shape))
        throw std::runtime_error(
            fmt::format("ONNX model input does not accept 1x3x{}x{}",
                        input_size, input_size));
    if (config->addOptimizationProfile(profile) < 0)
      throw std::runtime_error("Cannot add TensorRT optimization profile");
  } else if (!std::equal(dims.d, dims.d + 4, shape.d)) {
    throw std::runtime_error(fmt::format(
        "ONNX model input is fixed at {}x{}x{}x{}, which does not match "
        "input_size {}",
        dims.d[0], dims.d[1], dims.d[2], dims.d[3], input_size));
  }
  config->setProgressMonitor(&progress);

  std::unique_ptr<nvinfer1::IHostMemory> plan{
      builder->buildSerializedNetwork(*network, *config)};
  if (shutdown_requested)
    throw std::runtime_error("TensorRT engine build cancelled by shutdown");
  if (!plan || plan->size() == 0)
    throw std::runtime_error("TensorRT engine build failed for " +
                             model.string());
  std::ofstream file(output, std::ios::binary | std::ios::trunc);
  file.write(static_cast<const char *>(plan->data()),
             static_cast<std::streamsize>(plan->size()));
  file.close();
  if (!file)
    throw std::runtime_error("Cannot write " + output.string());
}

bool is_onnx(const fs::path &path) {
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return extension == ".onnx";
}

// A plan is published before its metadata, so a plan without metadata is an
// interrupted build and is rebuilt.
bool is_complete(const fs::path &plan, const fs::path &metadata) {
  std::error_code error;
  const auto size = fs::file_size(plan, error);
  return !error && size > 0 && fs::is_regular_file(metadata, error);
}
} // namespace

std::string resolve_tensorrt_plan(const std::string &model_path,
                                  int input_size) {
  const fs::path model(model_path);
  if (!is_onnx(model))
    return model_path;
  if (input_size <= 0)
    throw std::runtime_error("input_size must be positive");

  nlohmann::json metadata = fingerprint(model, input_size);
  const std::string identity = metadata.dump();
  const std::string key =
      fmt::format("{:016x}", fnv1a(identity.data(), identity.size()));
  const fs::path cache = model.parent_path() / ".engines";
  fs::create_directories(cache);
  const fs::path plan = cache / (key + ".engine");
  const fs::path metadata_path = cache / (key + ".json");

  const FileLock lock(cache / (key + ".lock"), plan);
  if (is_complete(plan, metadata_path)) {
    std::fprintf(stderr, "Using cached TensorRT engine: %s\n", plan.c_str());
    return plan.string();
  }

  std::fprintf(stderr,
               "Building TensorRT engine for %s on %s. This can take several "
               "minutes.\n",
               model.c_str(), metadata["gpu_name"].get<std::string>().c_str());
  // The lock makes this process the only writer for this key, so fixed
  // temporary names are safe; leftovers from a crash are overwritten.
  const fs::path temporary_plan = cache / (key + ".engine.tmp");
  const fs::path temporary_metadata = cache / (key + ".json.tmp");
  try {
    build_plan(model, input_size, temporary_plan);
    metadata["model_path"] = model_path;
    std::ofstream file(temporary_metadata, std::ios::trunc);
    file << metadata.dump(2) << '\n';
    file.close();
    if (!file)
      throw std::runtime_error("Cannot write " + temporary_metadata.string());
    fs::rename(temporary_plan, plan);
    fs::rename(temporary_metadata, metadata_path);
  } catch (...) {
    std::error_code ignored;
    fs::remove(temporary_plan, ignored);
    fs::remove(temporary_metadata, ignored);
    throw;
  }
  std::fprintf(stderr, "Saved TensorRT engine: %s\n", plan.c_str());
  return plan.string();
}
