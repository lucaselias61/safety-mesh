#pragma once

// Rerun builds only (CAM_NODE_WITH_RERUN).

#include "engines/engine.hpp"
#include "frames.hpp"
#include "geometry.hpp"
#include "logger.hpp"

#include <viewer_connection.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

/// Camera types to Rerun archetypes.
namespace viewer {

/// Pinhole model of a stream. RealSense axes: x right, y down, z forward.
rerun::Pinhole pinhole(const rs2_intrinsics &intrinsics);

/// Pose of one stream relative to another.
rerun::Transform3D transform(const rs2_extrinsics &extrinsics);

/// @return Capture time in ns since the epoch, from global time.
/// @throws std::runtime_error if the frame isn't in global time.
std::int64_t capture_time_ns(const rs2::frame &frame);

/// @return Color model for 8-bit RGB(A)/BGR(A), else std::nullopt.
std::optional<rerun::ColorModel> color_model(rs2_format format);

/// @return False if rows are padded.
bool is_packed(const rs2::video_frame &frame);

/// @brief Borrows the pixels: log before releasing the frame.
/// @pre Packed, and model matches the format.
rerun::Image color_image(const rs2::video_frame &frame,
                         rerun::ColorModel model);

/// Far end of the depth colormap, in meters. D455 ideal range ends near 6 m.
inline constexpr double depth_color_range_m = 6.0;

/// @brief Borrows the pixels: log before releasing the frame.
/// @pre Packed Z16.
rerun::DepthImage depth_image(const rs2::depth_frame &frame);

/// Boxes labelled with confidence.
rerun::Boxes2D boxes(const std::vector<Detection2D> &detections);

/// Keypoints of all detections.
rerun::Points2D keypoints(const std::vector<Detection2D> &detections);

/// COCO limbs of all detections, where both keypoints pass
/// keypoint_confidence_threshold.
rerun::LineStrips2D skeletons(const std::vector<Detection2D> &detections);

/// Radius of a 3D keypoint, in meters.
inline constexpr float keypoint_3d_radius_m = 2.0f;

/// 3D keypoints of all poses that have a point, in the color camera frame.
rerun::Points3D keypoints_3d(const std::vector<geometry::Detection3D> &poses);

/// Radius of a 3D limb, in meters.
inline constexpr float limb_3d_radius_m = 0.5f;

/// COCO limbs of all poses, where both keypoints have a point, in the color
/// camera frame.
rerun::LineStrips3D
skeletons_3d(const std::vector<geometry::Detection3D> &poses);

} // namespace viewer

/// @brief Rerun output for one camera, under cams/<serial>/.
/// @details Detections and timings every frame; images rate limited.
class ViewerSink {
public:
  using Warn = std::function<void(std::string_view)>;

  /// @param warn Receives one-time warnings about streams it can't show.
  ViewerSink(std::unique_ptr<rerun_logger::ViewerConnection> connection,
             Warn warn);

  /// Logs camera models, so depth renders as a point cloud in color's frame.
  void calibration(const geometry::Calibration &calibration);

  void frame(const Frames &frames, const std::vector<Detection2D> &detections,
             const std::vector<geometry::Detection3D> &poses,
             const LoopTimings &timings);

private:
  void timings(const LoopTimings &timings);
  void color(const rs2::video_frame &frame);
  void depth(const rs2::depth_frame &frame);
  void warn_once(bool &warned, std::string_view message);

  std::unique_ptr<rerun_logger::ViewerConnection> viewer_;
  Warn warn_;
  std::int64_t frame_index_ = 0;
  std::chrono::steady_clock::time_point last_frame_{};
  bool warned_color_format_ = false;
  bool warned_color_stride_ = false;
  bool warned_depth_format_ = false;
  bool warned_depth_stride_ = false;
};
