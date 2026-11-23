#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>

/// 3D point, meters.
using Point3 = std::array<double, 3>;

/// 4x4 rigid transform as rows, meters.
using Transform = std::array<std::array<double, 4>, 4>;

inline constexpr Transform identity_transform{
    {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};

/// @return a * b. Applies b first, then a.
// Same types by nature: the order is the matrix product.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Transform compose(const Transform &a, const Transform &b);

/// @return The point moved by the transform.
Point3 apply(const Transform &t, const Point3 &p);

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
