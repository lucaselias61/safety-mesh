#pragma once

#include <vector>
#include <ctime>
#include <librealsense2/rs.hpp>


struct Detection {
    std::vector<float> keypoints;
    std::vector<int> bbox;
    std::time_t timestamp;
    float confidence;
};


/// @brief Abstract base class for vision engines that perform object detection.
/// @details Derived classes should implement the detect method to process a frame and return a list of detections.
class Engine {
public:
    virtual ~Engine() = default;
    virtual std::vector<Detection> detect(const rs2::video_frame& frame) = 0;
};