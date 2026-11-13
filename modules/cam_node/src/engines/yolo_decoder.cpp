#include "engines/yolo_decoder.hpp"

#include <opencv2/dnn.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>

bool is_yolo_output(int channels) {
  return channels == yolo_detect_channels || channels == yolo_pose_channels;
}

std::vector<Detection> decode_yolo(const YoloOutput &output, float scale,
                                   cv::Size image, double confidence_threshold,
                                   double nms_threshold) {
  const int channels = output.channels;
  const int candidates = output.candidates;
  if (!is_yolo_output(channels))
    throw std::invalid_argument(
        "Expected YOLO output [1, 84, N] or [1, 56, N]");
  if (!output.data || candidates <= 0 || !(scale > 0) || image.width <= 0 ||
      image.height <= 0)
    throw std::invalid_argument("Invalid YOLO output dimensions");

  const bool pose = channels == yolo_pose_channels;
  const auto at = [&](int c, int i) {
    return output.data[static_cast<std::size_t>(c) * candidates + i];
  };
  const auto to_pixel = [scale](float v, int limit) {
    return cvRound(std::clamp(double(v) * scale, 0.0, double(limit)));
  };

  std::vector<cv::Rect> boxes;
  std::vector<float> scores;
  std::vector<std::vector<Keypoint>> keypoints;
  for (int i = 0; i < candidates; ++i) {
    const float cx = at(0, i), cy = at(1, i), w = at(2, i), h = at(3, i);
    const float score = at(4, i); // Pose: person score. Detection: class 0.
    if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(w) ||
        !std::isfinite(h) || !std::isfinite(score) ||
        score < confidence_threshold || w <= 0 || h <= 0)
      continue;
    // Clip in floating point before conversion to avoid overflowing pixel ints.
    const cv::Rect box(cv::Point(to_pixel(cx - w / 2, image.width),
                                 to_pixel(cy - h / 2, image.height)),
                       cv::Point(to_pixel(cx + w / 2, image.width),
                                 to_pixel(cy + h / 2, image.height)));
    if (box.empty())
      continue;

    std::vector<Keypoint> points;
    if (pose) {
      points.reserve(yolo_pose_keypoints);
      for (int k = 0; k < yolo_pose_keypoints; ++k) {
        const float x = at(5 + 3 * k, i), y = at(6 + 3 * k, i),
                    c = at(7 + 3 * k, i);
        // Keep the slot so the index still maps to the COCO body part.
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(c))
          points.push_back({0, 0, 0.0f});
        else
          points.push_back(
              {to_pixel(x, image.width - 1), to_pixel(y, image.height - 1), c});
      }
    }
    boxes.push_back(box);
    scores.push_back(score);
    keypoints.push_back(std::move(points));
  }

  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, scores, static_cast<float>(confidence_threshold),
                    static_cast<float>(nms_threshold), indices);
  std::vector<Detection> detections;
  detections.reserve(indices.size());
  const auto timestamp = std::time(nullptr);
  for (int i : indices)
    detections.push_back(
        {std::move(keypoints[i]), {boxes[i], scores[i]}, timestamp});
  return detections;
}
