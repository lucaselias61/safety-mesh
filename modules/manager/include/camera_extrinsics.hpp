#pragma once

#include <array>
#include <chrono>
#include <filesystem>
#include <string>

/// 4x4 rigid transform as rows, meters.
using Transform = std::array<std::array<double, 4>, 4>;

/// Pose of one camera in the world (board) frame, from cam_node calibrate.
struct CameraExtrinsics {
  std::string serial;
  /// Maps a point in the color camera frame (x right, y down, z forward) to
  /// the world frame.
  Transform camera_to_world;
  double reprojection_error_px;
  std::chrono::system_clock::time_point estimated_at;
};

/// @brief Loads a camera pose file written by `cam_node calibrate --out`.
/// @param expected_serial Serial the file must belong to.
/// @throws std::runtime_error if the file cannot be read, a key is missing,
/// the serial differs, the last row is not 0 0 0 1 or the rotation is not a
/// proper rotation.
CameraExtrinsics load_camera_extrinsics(const std::filesystem::path &path,
                                        const std::string &expected_serial);
