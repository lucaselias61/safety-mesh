#pragma once

#include <librealsense2/rs.hpp>
#include <opencv2/core.hpp>

#include <optional>

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

} // namespace geometry
