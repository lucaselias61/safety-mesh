#include "geometry.hpp"

#include <librealsense2/rsutil.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace geometry {

cv::Point3f deproject(const rs2_intrinsics &intrinsics,
                      const cv::Point2f &pixel, float depth) {
  const float from[2] = {pixel.x, pixel.y};
  float to[3];
  rs2_deproject_pixel_to_point(to, &intrinsics, from, depth);
  return {to[0], to[1], to[2]};
}

cv::Point2f project(const rs2_intrinsics &intrinsics,
                    const cv::Point3f &point) {
  const float from[3] = {point.x, point.y, point.z};
  float to[2];
  rs2_project_point_to_pixel(to, &intrinsics, from);
  return {to[0], to[1]};
}

cv::Point3f transform(const rs2_extrinsics &extrinsics,
                      const cv::Point3f &point) {
  const float from[3] = {point.x, point.y, point.z};
  float to[3];
  rs2_transform_point_to_point(to, &extrinsics, from);
  return {to[0], to[1], to[2]};
}

std::optional<cv::Point2f> color_to_depth_pixel(const rs2::depth_frame &depth,
                                                const cv::Point2f &color_pixel,
                                                const Calibration &calibration,
                                                float depth_min,
                                                float depth_max) {
  const auto &intr = calibration.intrinsics;
  const auto &extr = calibration.extrinsics;
  if (depth.get_profile().format() != RS2_FORMAT_Z16)
    throw std::invalid_argument("Expected a Z16 depth frame");
  // rsutil indexes the buffer with the intrinsics' width, so a mismatch would
  // read out of bounds.
  if (depth.get_width() != intr.depth.width ||
      depth.get_height() != intr.depth.height)
    throw std::invalid_argument(
        "Depth frame resolution does not match the depth intrinsics");
  const float from[2] = {color_pixel.x, color_pixel.y};
  // Pre-fill out of bounds so a pixel that was never written is rejected.
  float to[2] = {-1.0f, -1.0f};
  rs2_project_color_pixel_to_depth_pixel(
      to, static_cast<const uint16_t *>(depth.get_data()), depth.get_units(),
      depth_min, depth_max, &intr.depth, &intr.color, &extr.color_to_depth,
      &extr.depth_to_color, from);
  if (to[0] < 0 || to[1] < 0 ||
      to[0] >= static_cast<float>(depth.get_width()) ||
      to[1] >= static_cast<float>(depth.get_height()))
    return std::nullopt;
  return cv::Point2f(to[0], to[1]);
}

std::optional<cv::Point3f>
color_pixel_to_point(const rs2::depth_frame &depth,
                     const cv::Point2f &color_pixel,
                     const Calibration &calibration) {
  const auto depth_pixel =
      color_to_depth_pixel(depth, color_pixel, calibration);
  if (!depth_pixel)
    return std::nullopt;
  // Nearest pixel. Rounding can step one past the last row or column.
  const int x = std::min(static_cast<int>(std::lround(depth_pixel->x)),
                         depth.get_width() - 1);
  const int y = std::min(static_cast<int>(std::lround(depth_pixel->y)),
                         depth.get_height() - 1);
  // 0 means no depth at this pixel.
  const float z = depth.get_distance(x, y);
  if (z <= 0.0f)
    return std::nullopt;
  const cv::Point3f in_depth =
      deproject(calibration.intrinsics.depth,
                {static_cast<float>(x), static_cast<float>(y)}, z);
  return transform(calibration.extrinsics.depth_to_color, in_depth);
}

Detection3D detection_in_3d(const Detection2D &detection,
                            const rs2::depth_frame &depth,
                            const Calibration &calibration,
                            std::chrono::system_clock::time_point timestamp) {
  Detection3D result{{}, detection.bbox.confidence, timestamp};
  result.keypoints.reserve(detection.keypoints.size());
  for (const auto &keypoint : detection.keypoints) {
    Keypoint3D lifted{std::nullopt, keypoint.confidence};
    if (keypoint.confidence >= keypoint_confidence_threshold)
      lifted.point = color_pixel_to_point(
          depth,
          {static_cast<float>(keypoint.x), static_cast<float>(keypoint.y)},
          calibration);
    result.keypoints.push_back(lifted);
  }
  return result;
}

} // namespace geometry
