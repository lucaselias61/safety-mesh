#include "engines/opencv_dnn_engine.hpp"

#include <algorithm>
#include <stdexcept>

namespace {

cv::Mat create_padded_image(const cv::Mat& image, int img_size) {
    cv::Mat padded(img_size, img_size, CV_8UC3, cv::Scalar(114, 114, 114));
    image.copyTo(padded(cv::Rect(0, 0, image.cols, image.rows)));
    return padded;
}

}

OpenCVDnnEngine::OpenCVDnnEngine(const EngineConfig& config)
    : net_(cv::dnn::readNetFromONNX(config.model_path)), confidence_threshold_(config.confidence_threshold), 
      nms_threshold_(config.nms_threshold), input_size_(config.input_size) {
    net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

std::vector<Detection> OpenCVDnnEngine::detect(const rs2::video_frame& frame) {
    if (!frame) return {};
    const auto format = frame.get_profile().format();
    if (format != RS2_FORMAT_RGB8 && format != RS2_FORMAT_BGR8){
        throw std::invalid_argument("Expected an RGB8 or BGR8 color frame");
    }

    cv::Mat image(frame.get_height(), frame.get_width(), CV_8UC3, const_cast<void*>(frame.get_data()), frame.get_stride_in_bytes());
    const int img_size = std::max(image.cols, image.rows);
    cv::Mat padded = create_padded_image(image, img_size);

    net_.setInput(cv::dnn::blobFromImage(padded, 1.0 / 255.0, cv::Size(input_size_, input_size_),cv::Scalar(), format == RS2_FORMAT_BGR8, false));
    cv::Mat output = net_.forward();
    // Standard COCO YOLOv8/YOLO11 detection export: [1, 84, N].
    if (output.dims != 3 || output.size[0] != 1 || output.size[1] != 84)
        throw std::runtime_error("Expected YOLO detection output [1, 84, N]");
    cv::Mat predictions;
    cv::transpose(cv::Mat(84, output.size[2], CV_32F, output.ptr<float>()), predictions);

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    const float scale = img_size / static_cast<float>(input_size_);
    for (int i = 0; i < predictions.rows; ++i) {
        const float* p = predictions.ptr<float>(i);
        if (p[4] < confidence_threshold_) continue; // COCO class 0: person.
        cv::Rect box(cvRound((p[0] - p[2] / 2) * scale),
                     cvRound((p[1] - p[3] / 2) * scale),
                     cvRound(p[2] * scale), cvRound(p[3] * scale));
        box &= cv::Rect(0, 0, image.cols, image.rows);
        if (box.empty()) continue;
        boxes.push_back(box);
        scores.push_back(p[4]);
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, confidence_threshold_, nms_threshold_, indices);
    std::vector<Detection> detections;
    const auto timestamp = std::time(nullptr);
    for (int i : indices) {
        const auto& box = boxes[i];
        // bbox uses pixel coordinates: [x_min, y_min, x_max, y_max].
        detections.push_back({{}, {box.x, box.y, box.x + box.width, box.y + box.height},
                              timestamp, scores[i]});
    }
    return detections;
}

