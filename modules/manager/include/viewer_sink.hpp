#pragma once

// Rerun builds only (MANAGER_WITH_RERUN).

#include "logger.hpp"
#include "transform.hpp"

#include <viewer_connection.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace viewer {

/// Pose of a child frame (e.g. a camera) in its parent (the world).
rerun::Transform3D transform(const Transform &child_to_parent);

/// Radius of a robot joint, in meters.
inline constexpr float joint_radius_m = 0.03f;
/// Radius of a robot link, in meters.
inline constexpr float link_radius_m = 0.015f;
/// Radius of the robot TCP, in meters.
inline constexpr float tcp_radius_m = 0.02f;

} // namespace viewer

/// @brief Rerun output of the manager, in the world frame.
/// @details
/// - cams/<serial>: static camera pose. cam_node logs under the same path, so
/// its images, point clouds and keypoints move into the world.
/// - robots/<id>: joints, links and TCP every poll.
class ViewerSink {
public:
  explicit ViewerSink(
      std::unique_ptr<rerun_logger::ViewerConnection> connection);

  void cameras(const std::vector<CameraConfig> &cameras);
  void robot(const RobotState &state);

private:
  std::unique_ptr<rerun_logger::ViewerConnection> viewer_;
  std::int64_t frame_index_ = 0;
};
