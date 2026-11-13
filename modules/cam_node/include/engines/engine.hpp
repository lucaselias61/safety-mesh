#pragma once

#include <ctime>
#include <librealsense2/rs.hpp>
#include <opencv2/core.hpp>
#include <vector>

/// Keypoints below this confidence were guessed by the model (e.g. occluded).
inline constexpr float keypoint_confidence_threshold = 0.5f;

/// Pixel position in the color image and model confidence in [0, 1].
struct Keypoint {
  int x;
  int y;
  float confidence;
};

/// Bounding box in color image pixels and its model confidence in [0, 1].
struct BBox {
  cv::Rect rect;
  float confidence;
};

struct Detection {
  std::vector<Keypoint> keypoints; // COCO order; empty for detection models.
  BBox bbox;
  std::time_t timestamp;
};

/// @brief Abstract base class for vision engines that perform object detection.
/// @details Derived classes should implement the detect method to process a
/// frame and return a list of detections.
class Engine {
public:
  virtual ~Engine() = default;
  virtual std::vector<Detection> detect(const rs2::video_frame &frame) = 0;
};