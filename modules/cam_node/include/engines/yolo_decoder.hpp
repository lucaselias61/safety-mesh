#pragma once

#include "engines/engine.hpp"

#include <opencv2/core.hpp>
#include <vector>

// YOLO based output channels per candidate, without embedded NMS.
// Detection: 4 for box + 80 COCO class scores (0 = person).
inline constexpr int yolo_detect_channels = 84;
// Pose: 4 box + 1 person score + 17 COCO keypoints x 3 (x, y, confidence).
inline constexpr int yolo_pose_keypoints = 17;
inline constexpr int yolo_pose_channels = 5 + 3 * yolo_pose_keypoints;

/// @brief True if supported YOLO output.
bool is_yolo_output(int channels);

/// Raw output [1, channels, candidates]
/// channel value x of candidate i is at data[x * candidates + i].
struct YoloOutput {
  const float *data;
  int channels;
  int candidates;
};

/// @brief Decodes a raw YOLO output [1, C, N] into person detections.
/// @details C = 84: detection model, keypoints left empty.
///          C = 56: pose model, 17 COCO keypoints per detection, in COCO
///          order. Every keypoint is kept; check its confidence before use.
/// @param scale      Padded square image size / model input size.
/// @param image      Original image size, used to clip boxes and keypoints.
/// @throws std::invalid_argument if unsupported channels or candidates,
std::vector<Detection> decode_yolo(const YoloOutput &output, float scale,
                                   cv::Size image, double confidence_threshold,
                                   double nms_threshold);
