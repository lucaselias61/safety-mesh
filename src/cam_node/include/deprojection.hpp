#pragma once

#include "camera.hpp"

#include <opencv2/core.hpp>

/// @brief Computes the depth value at a given color pixel using the camera
/// intrinsics.
float depth_from_color_pixel(const Intrinsics &intrinsics,
                             const cv::Point2f &pixel);

/// @brief Deprojects a pixel to a 3D point using the camera intrinsics and
/// depth.
/// @param intrinsics The camera intrinsics matrix.
/// @param pixel The 2D pixel coordinates.
/// @param depth The depth value at the pixel.
/// @return The corresponding 3D point in camera coordinates.
cv::Point3f deproject_pixel_to_point(const cv::Mat &intrinsics,
                                     const cv::Point2f &pixel, float depth);
