#include "vision.hpp"

#include <librealsense2/rs.hpp>


const Detection& Inference::detect(const rs2::video_frame& frame) {
    static Detection detection;
    /// Populate the detection object with the results of the inference
    detection.timestamp = std::time(nullptr);
    return detection;
}


