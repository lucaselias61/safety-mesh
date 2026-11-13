// CPU implementation of the OpenCVDnnEngine using OpenCV's DNN module.
#include "engines/opencv_dnn_engine.hpp"

#include "engines/yolo_decoder.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace {

cv::Mat create_padded_image(const cv::Mat &image, int img_size) {
  cv::Mat padded(img_size, img_size, CV_8UC3, cv::Scalar(114, 114, 114));
  image.copyTo(padded(cv::Rect(0, 0, image.cols, image.rows)));
  return padded;
}

} // namespace

OpenCVDnnEngine::OpenCVDnnEngine(const EngineConfig &config)
    : net_(cv::dnn::readNetFromONNX(config.model_path)),
      confidence_threshold_(config.confidence_threshold),
      nms_threshold_(config.nms_threshold), input_size_(config.input_size) {
  if (config.precision != Precision::fp32)
    throw std::invalid_argument(
        std::string("opencv_dnn supports only fp32 precision, not ") +
        to_string(config.precision));
  net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
  net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

std::vector<Detection> OpenCVDnnEngine::detect(const rs2::video_frame &frame) {
  if (!frame)
    return {};
  const auto format = frame.get_profile().format();
  if (format != RS2_FORMAT_RGB8 && format != RS2_FORMAT_BGR8) {
    throw std::invalid_argument("Expected an RGB8 or BGR8 color frame");
  }

  cv::Mat image(frame.get_height(), frame.get_width(), CV_8UC3,
                const_cast<void *>(frame.get_data()),
                frame.get_stride_in_bytes());
  const int img_size = std::max(image.cols, image.rows);
  cv::Mat padded = create_padded_image(image, img_size);

  net_.setInput(cv::dnn::blobFromImage(
      padded, 1.0 / 255.0, cv::Size(input_size_, input_size_), cv::Scalar(),
      format == RS2_FORMAT_BGR8, false));
  cv::Mat output = net_.forward();
  // YOLOv8/YOLO11 export without NMS: [1, 84, N] detection or [1, 56, N] pose.
  if (output.type() != CV_32F || output.dims != 3 || output.size[0] != 1 ||
      !is_yolo_output(output.size[1]) || output.size[2] <= 0 ||
      !output.isContinuous())
    throw std::runtime_error("Expected YOLO output [1, 84, N] or [1, 56, N]");
  const float scale =
      static_cast<float>(img_size) / static_cast<float>(input_size_);
  return decode_yolo({output.ptr<float>(), output.size[1], output.size[2]},
                     scale, image.size(), confidence_threshold_,
                     nms_threshold_);
}
