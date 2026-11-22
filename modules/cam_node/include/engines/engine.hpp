#pragma once

#include <librealsense2/rs.hpp>
#include <opencv2/core.hpp>

#include <array>
#include <utility>
#include <vector>

/// Keypoints below this confidence were guessed by the model (e.g. occluded).
inline constexpr float keypoint_confidence_threshold = 0.5f;

/// Keypoints of a COCO pose, in COCO order.
/// - 0 nose, 1-2 eyes, 3-4 ears, 5-6 shoulders, 7-8 elbows, 9-10 wrists,
/// 11-12 hips, 13-14 knees, 15-16 ankles. Left before right.
inline constexpr int coco_keypoints = 17;

/// COCO limbs, as pairs of keypoint indices.
inline constexpr std::array<std::pair<int, int>, 19> coco_skeleton{{
    {15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, // Legs and hips.
    {5, 11},  {6, 12},  {5, 6},                       // Torso.
    {5, 7},   {7, 9},   {6, 8},   {8, 10},            // Arms.
    {1, 2},   {0, 1},   {0, 2},   {1, 3},   {2, 4},   // Face.
    {3, 5},   {4, 6},                                 // Ears to shoulders.
}};

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

struct Detection2D {
  std::vector<Keypoint> keypoints; // COCO order; empty for detection models.
  BBox bbox;
};

/// @brief Abstract base class for vision engines that perform object detection.
/// @details Derived classes should implement the detect method to process a
/// frame and return a list of detections.
class Engine {
public:
  virtual ~Engine() = default;
  virtual std::vector<Detection2D> detect(const rs2::video_frame &frame) = 0;
};