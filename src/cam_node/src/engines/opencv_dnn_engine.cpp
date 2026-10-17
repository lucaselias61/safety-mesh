#include "engines/opencv_dnn_engine.hpp"

OpenCVDnnEngine::OpenCVDnnEngine(const std::string& modelPath)
    : net_(cv::dnn::readNet(modelPath)) {
}

std::vector<Detection> OpenCVDnnEngine::detect(const rs2::video_frame& frame) {
    // Implementation for running the DNN on the given frame
    std::vector<Detection> detections;
    // TODO: Add Inference
    return detections;
}

