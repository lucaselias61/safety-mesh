#pragma once

#include "engines/engine.hpp"

#include <librealsense2/rs.hpp>
#include <opencv2/core.hpp>

#include <chrono>
#include <optional>
#include <vector>

/// Camera geometry helpers: thin wrappers around librealsense's rsutil.h.
/// Points are in meters, in the frame of the camera whose intrinsics are used.
namespace geometry {

/// Depth and color intrinsics of the active streams.
struct Intrinsics {
  rs2_intrinsics depth;
  rs2_intrinsics color;
};

/// Rigid transforms between the depth and color sensors.
struct Extrinsics {
  rs2_extrinsics color_to_depth;
  rs2_extrinsics depth_to_color;
};

/// Calibration of a running depth + color stream pair.
struct Calibration {
  Intrinsics intrinsics;
  Extrinsics extrinsics;
};

/// 3D keypoint and its model confidence in [0, 1].
struct Keypoint3D {
  // Color camera frame, in meters.
  // Empty if low confidence, no depth or outside the depth frame.
  std::optional<cv::Point3f> point;
  float confidence;
};

/// Person pose in 3D, from a Detection2D and the depth frame.
struct Detection3D {
  std::vector<Keypoint3D> keypoints; // COCO order, as in Detection2D.
  float confidence;                  // Person score in [0, 1].
  std::chrono::system_clock::time_point timestamp; // Color capture time.
};

/// @brief Deprojects a pixel and its depth to a 3D point.
/// @param intrinsics Intrinsics of the camera that produced the depth (use the
/// color intrinsics if the depth frame is aligned to color).
/// @param pixel The 2D pixel coordinates.
/// @param depth The Z distance at the pixel, in meters.
/// @return The 3D point in that camera's frame.
cv::Point3f deproject(const rs2_intrinsics &intrinsics,
                      const cv::Point2f &pixel, float depth);

/// @brief Projects a 3D point to a pixel.
/// @param intrinsics Intrinsics of the camera the point is expressed in.
/// @param point The 3D point in that camera's frame, in meters.
/// @return The 2D pixel coordinates.
cv::Point2f project(const rs2_intrinsics &intrinsics, const cv::Point3f &point);

/// @brief Moves a 3D point from one sensor's frame to another's.
/// @param extrinsics Transform from the source to the target frame, e.g. from
/// depth_profile.get_extrinsics_to(color_profile).
/// @param point The 3D point in the source frame.
/// @return The 3D point in the target frame.
cv::Point3f transform(const rs2_extrinsics &extrinsics,
                      const cv::Point3f &point);

/// @brief Finds the depth pixel that sees the same scene point as a color
/// pixel, without aligning the whole frame.
/// @details Searches along the depth image for the depth range
/// [depth_min, depth_max]. Use it for a few pixels (e.g. keypoints); for
/// whole images use rs2::align instead.
/// @param depth The unaligned Z16 depth frame.
/// @param color_pixel The 2D color pixel coordinates.
/// @param calibration Calibration of the streams that produced the frames.
/// @param depth_min Nearest depth to search, in meters.
/// @param depth_max Farthest depth to search, in meters.
/// @return The depth pixel, or std::nullopt if no match lies inside the frame.
/// @throws std::invalid_argument if the frame is not Z16 or its resolution
/// does not match the depth intrinsics.
std::optional<cv::Point2f> color_to_depth_pixel(const rs2::depth_frame &depth,
                                                const cv::Point2f &color_pixel,
                                                const Calibration &calibration,
                                                float depth_min = 0.1f,
                                                float depth_max = 10.0f);

/// @brief 3D point a color pixel sees, in the color camera frame, in meters.
/// - Reads the nearest depth pixel from color_to_depth_pixel(). No filtering.
/// @return std::nullopt if no depth pixel matches or it has no depth.
/// @throws std::invalid_argument as color_to_depth_pixel().
std::optional<cv::Point3f> color_pixel_to_point(const rs2::depth_frame &depth,
                                                const cv::Point2f &color_pixel,
                                                const Calibration &calibration);

/// @brief 3D pose of a detection, keypoints in the color camera frame.
/// - Keypoints below keypoint_confidence_threshold get no point.
/// @param timestamp Capture time of the color frame the detection is from.
/// @throws std::invalid_argument as color_to_depth_pixel().
Detection3D detection_in_3d(const Detection2D &detection,
                            const rs2::depth_frame &depth,
                            const Calibration &calibration,
                            std::chrono::system_clock::time_point timestamp);

} // namespace geometry
