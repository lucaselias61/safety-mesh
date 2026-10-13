#pragma once

#include "pipeline.hpp"

#include <vector>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <ctime>


class Detection {
public:
    std::vector<float> keypoints;
    std::vector<int> bbox;
    float confidence;
    std::time_t timestamp;
};


class Inference {
public:
    explicit Inference(const Pipeline& pipeline) 
        : pipeline_(pipeline) {};

    const Detection& detect(const rs2::video_frame& frame);

private:
    Pipeline pipeline_;
};
