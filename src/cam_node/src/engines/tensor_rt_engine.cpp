#include "engines/tensor_rt_engine.hpp"

#include <NvInfer.h>
#include <cuda_runtime_api.h>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

#if NV_TENSORRT_MAJOR < 10
#error "TensorRTEngine requires TensorRT 10 or newer"
#endif

namespace {
void handle_cuda_error(cudaError_t result) {
  if (result != cudaSuccess)
    throw std::runtime_error(std::string("CUDA: ") +
                             cudaGetErrorString(result));
}

struct Logger : nvinfer1::ILogger {
  void log(Severity severity, const char *message) noexcept override {
    if (severity <= Severity::kWARNING)
      std::fprintf(stderr, "TensorRT: %s\n", message);
  }
};

// RAII wrapper for a CUDA stream, which is a stream of operations for the GPU.
// Ensures proper creation, synchronization, and destruction.
struct CudaStream {
  cudaStream_t value{};
  CudaStream() {
    handle_cuda_error(cudaStreamCreateWithFlags(&value, cudaStreamNonBlocking));
  }
  ~CudaStream() {
    if (value) {
      cudaStreamSynchronize(value);
      cudaStreamDestroy(value);
    }
  }
  // Block copying and assignment to ensure unique ownership of the CUDA stream.
  // "= delete" tells the compiler the operation is illegal.
  CudaStream(const CudaStream &) = delete;
  CudaStream &operator=(const CudaStream &) = delete;
};

struct Buffer {
  void *data = nullptr;
  ~Buffer() {
    if (data)
      cudaFree(data);
  }
  void allocate(std::size_t bytes) {
    handle_cuda_error(cudaMalloc(&data, bytes));
  }
  Buffer() = default;
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;
};

std::size_t byte_size(const nvinfer1::Dims &dims) {
  std::size_t bytes = sizeof(float);
  for (int i = 0; i < dims.nbDims; ++i) {
    if (dims.d[i] <= 0 || static_cast<std::size_t>(dims.d[i]) >
                              std::numeric_limits<std::size_t>::max() / bytes)
      throw std::runtime_error("Unresolved or oversized TensorRT tensor shape");
    bytes *= static_cast<std::size_t>(dims.d[i]);
  }
  return bytes;
}
} // namespace

struct TensorRTEngine::Impl {
  // Reverse destruction keeps the logger and runtime alive for their
  // dependents.
  Logger logger;
  std::unique_ptr<nvinfer1::IRuntime> runtime;
  std::unique_ptr<nvinfer1::ICudaEngine> engine;
  std::unique_ptr<nvinfer1::IExecutionContext> context;
  Buffer input;
  Buffer output;
  std::vector<float> predictions;
  CudaStream
      stream; // Synchronize before freeing buffers, including on exceptions.
  std::string input_name;
  std::string output_name;
  std::size_t input_bytes = 0;
  std::size_t output_bytes = 0;
  int candidates = 0;
  int input_size;
  double confidence_threshold;
  double nms_threshold;

  explicit Impl(const EngineConfig &config)
      : input_size(config.input_size),
        confidence_threshold(config.confidence_threshold),
        nms_threshold(config.nms_threshold) {
    if (input_size <= 0 || !std::isfinite(confidence_threshold) ||
        confidence_threshold < 0 || confidence_threshold > 1 ||
        !std::isfinite(nms_threshold) || nms_threshold < 0 || nms_threshold > 1)
      throw std::invalid_argument("Invalid TensorRT detection configuration");
    std::ifstream file(config.model_path, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open TensorRT plan: " +
                               config.model_path);
    std::vector<char> plan{std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>()};
    if (file.bad() || plan.empty())
      throw std::runtime_error("Cannot read TensorRT plan: " +
                               config.model_path);
    runtime.reset(nvinfer1::createInferRuntime(logger));
    if (!runtime)
      throw std::runtime_error("Cannot create TensorRT runtime");
    engine.reset(runtime->deserializeCudaEngine(plan.data(), plan.size()));
    if (!engine)
      throw std::runtime_error("Cannot deserialize TensorRT plan");
    if (engine->getNbIOTensors() != 2)
      throw std::runtime_error("Expected one TensorRT input and one output");
    for (int i = 0; i < engine->getNbIOTensors(); ++i) {
      const char *name = engine->getIOTensorName(i);
      if (engine->getTensorDataType(name) != nvinfer1::DataType::kFLOAT ||
          engine->getTensorFormat(name) != nvinfer1::TensorFormat::kLINEAR ||
          engine->getTensorLocation(name) != nvinfer1::TensorLocation::kDEVICE)
        throw std::runtime_error("Expected FP32 linear device tensors");
      if (engine->getTensorIOMode(name) == nvinfer1::TensorIOMode::kINPUT)
        input_name = name;
      else
        output_name = name;
    }
    if (input_name.empty() || output_name.empty())
      throw std::runtime_error("Expected one TensorRT input and one output");
    context.reset(engine->createExecutionContext());
    if (!context)
      throw std::runtime_error("Cannot create TensorRT context");
    const nvinfer1::Dims4 shape{1, 3, input_size, input_size};
    if (!context->setInputShape(input_name.c_str(), shape))
      throw std::runtime_error(
          "TensorRT input shape does not match configuration");
    const auto dims = context->getTensorShape(output_name.c_str());
    if (dims.nbDims != 3 || dims.d[0] != 1 || dims.d[1] != 84 ||
        dims.d[2] <= 0 || dims.d[2] > std::numeric_limits<int>::max())
      throw std::runtime_error("Expected YOLO detection output [1, 84, N]");
    candidates = static_cast<int>(dims.d[2]);
    input_bytes = byte_size(shape);
    output_bytes = byte_size(dims);
    input.allocate(input_bytes);
    output.allocate(output_bytes);
    predictions.resize(output_bytes / sizeof(float));
    if (!context->setTensorAddress(input_name.c_str(), input.data) ||
        !context->setTensorAddress(output_name.c_str(), output.data))
      throw std::runtime_error("Cannot bind TensorRT tensors");
  }
};

TensorRTEngine::TensorRTEngine(const EngineConfig &config)
    : impl_(std::make_unique<Impl>(config)) {}
TensorRTEngine::~TensorRTEngine() = default;

std::vector<Detection> TensorRTEngine::detect(const rs2::video_frame &frame) {
  if (!frame)
    return {};
  auto &state = *impl_;
  const auto format = frame.get_profile().format();
  if (format != RS2_FORMAT_RGB8 && format != RS2_FORMAT_BGR8)
    throw std::invalid_argument("Expected an RGB8 or BGR8 color frame");
  cv::Mat image(frame.get_height(), frame.get_width(), CV_8UC3,
                const_cast<void *>(frame.get_data()),
                frame.get_stride_in_bytes());
  const int img_size = std::max(image.cols, image.rows);
  cv::Mat padded(img_size, img_size, CV_8UC3, cv::Scalar(114, 114, 114));
  image.copyTo(padded(cv::Rect(0, 0, image.cols, image.rows)));
  cv::Mat blob = cv::dnn::blobFromImage(
      padded, 1.0 / 255.0, cv::Size(state.input_size, state.input_size),
      cv::Scalar(), format == RS2_FORMAT_BGR8, false);
  // Ensure queued work finishes before local host memory is released on
  // failure.
  try {
    check_cuda(cudaMemcpyAsync(state.input.data, blob.ptr<float>(),
                               state.input_bytes, cudaMemcpyHostToDevice,
                               state.stream.value));
    if (!state.context->enqueueV3(state.stream.value))
      throw std::runtime_error("TensorRT enqueueV3 failed");
    check_cuda(cudaMemcpyAsync(state.predictions.data(), state.output.data,
                               state.output_bytes, cudaMemcpyDeviceToHost,
                               state.stream.value));
    check_cuda(cudaStreamSynchronize(state.stream.value));
  } catch (...) {
    cudaStreamSynchronize(state.stream.value);
    throw;
  }
  cv::Mat predictions;
  cv::transpose(cv::Mat(84, state.candidates, CV_32F, state.predictions.data()),
                predictions);
  std::vector<cv::Rect> boxes;
  std::vector<float> scores;
  const float scale = static_cast<float>(img_size) / state.input_size;
  for (int i = 0; i < predictions.rows; ++i) {
    const float *p = predictions.ptr<float>(i);
    if (!std::all_of(p, p + 5, [](float v) { return std::isfinite(v); }) ||
        p[4] < state.confidence_threshold || p[2] <= 0 || p[3] <= 0)
      continue;
    // Clip in floating point before conversion to avoid overflowing pixel ints.
    const double left = std::clamp((double(p[0]) - p[2] / 2.0) * scale, 0.0,
                                   double(image.cols));
    const double top = std::clamp((double(p[1]) - p[3] / 2.0) * scale, 0.0,
                                  double(image.rows));
    const double right = std::clamp((double(p[0]) + p[2] / 2.0) * scale, 0.0,
                                    double(image.cols));
    const double bottom = std::clamp((double(p[1]) + p[3] / 2.0) * scale, 0.0,
                                     double(image.rows));
    cv::Rect box(cv::Point(cvRound(left), cvRound(top)),
                 cv::Point(cvRound(right), cvRound(bottom)));
    if (box.empty())
      continue;
    boxes.push_back(box);
    scores.push_back(p[4]); // COCO class 0: person.
  }
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, scores,
                    static_cast<float>(state.confidence_threshold),
                    static_cast<float>(state.nms_threshold), indices);
  std::vector<Detection> detections;
  const auto timestamp = std::time(nullptr);
  for (int i : indices)
    detections.push_back({{}, boxes[i], timestamp, scores[i]});
  return detections;
}
