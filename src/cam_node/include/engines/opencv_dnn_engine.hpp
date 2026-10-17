#pragma once

#include "engine.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>


class OpenCVDnnEngine : public Engine {
public:
    explicit OpenCVDnnEngine(const std::string& modelPath);

    std::vector<Detection> detect(const rs2::video_frame& frame) override;

private:
    cv::dnn::Net net_;
};