#pragma once

// Rerun builds only (CAM_NODE_WITH_RERUN).

#include "engines/engine.hpp"
#include "geometry.hpp"
#include "logger.hpp"
#include "pipeline.hpp"

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

/// @return Capture time in ns since the epoch. Uses the host clock (time of
/// logging) if the frame isn't timestamped in host time.
std::int64_t capture_time_ns(const rs2::frame &frame);

/// @return Color model for 8-bit RGB(A)/BGR(A), else std::nullopt.
std::optional<rerun::ColorModel> color_model(rs2_format format);

/// @return False if rows are padded.
bool is_packed(const rs2::video_frame &frame);

/// @brief Borrows the pixels: log before releasing the frame.
/// @pre Packed, and model matches the format.
rerun::Image color_image(const rs2::video_frame &frame,
                         rerun::ColorModel model);

/// @brief Borrows the pixels: log before releasing the frame.
/// @pre Packed Z16.
rerun::DepthImage depth_image(const rs2::depth_frame &frame);

/// Boxes labelled with confidence.
rerun::Boxes2D boxes(const std::vector<Detection> &detections);

/// Keypoints of all detections.
rerun::Points2D keypoints(const std::vector<Detection> &detections);

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

  void frame(const Frames &frames, const std::vector<Detection> &detections,
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
