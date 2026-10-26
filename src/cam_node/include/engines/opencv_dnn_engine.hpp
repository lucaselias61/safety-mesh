#pragma once

#include "engine.hpp"
#include "utils.hpp"

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

class OpenCVDnnEngine : public Engine {
public:
  explicit OpenCVDnnEngine(const EngineConfig &config);

  std::vector<Detection> detect(const rs2::video_frame &frame) override;

private:
  cv::dnn::Net net_;
  double confidence_threshold_;
  double nms_threshold_;
  int input_size_;
};