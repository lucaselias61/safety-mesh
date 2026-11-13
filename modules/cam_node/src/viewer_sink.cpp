#include "viewer_sink.hpp"

#include <fmt/format.h>

#include <array>
#include <chrono>
#include <utility>

namespace viewer {

rerun::Pinhole pinhole(const rs2_intrinsics &intrinsics) {
  // Column-major 3x3 projection matrix.
  const std::array<float, 9> image_from_camera{
      intrinsics.fx,  0.0f,           0.0f, //
      0.0f,           intrinsics.fy,  0.0f, //
      intrinsics.ppx, intrinsics.ppy, 1.0f};
  return rerun::Pinhole(rerun::components::PinholeProjection(image_from_camera))
      .with_resolution(intrinsics.width, intrinsics.height)
      .with_camera_xyz(rerun::components::ViewCoordinates::RDF);
}

rerun::Transform3D transform(const rs2_extrinsics &extrinsics) {
  std::array<float, 9> rotation{}; // rs2 rotation is column-major too.
  for (std::size_t i = 0; i < rotation.size(); ++i)
    rotation[i] = extrinsics.rotation[i];
  return rerun::Transform3D::from_translation_mat3x3(
      rerun::components::Translation3D(extrinsics.translation),
      rerun::components::TransformMat3x3(rotation));
}

std::int64_t capture_time_ns(const rs2::frame &frame) {
  const auto domain = frame.get_frame_timestamp_domain();
  if (domain == RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME ||
      domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
    return static_cast<std::int64_t>(frame.get_timestamp() * 1e6);
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::optional<rerun::ColorModel> color_model(rs2_format format) {
  switch (format) {
  case RS2_FORMAT_RGB8:
    return rerun::ColorModel::RGB;
  case RS2_FORMAT_BGR8:
    return rerun::ColorModel::BGR;
  case RS2_FORMAT_RGBA8:
    return rerun::ColorModel::RGBA;
  case RS2_FORMAT_BGRA8:
    return rerun::ColorModel::BGRA;
  default:
    return std::nullopt;
  }
}

bool is_packed(const rs2::video_frame &frame) {
  return frame.get_stride_in_bytes() ==
         frame.get_width() * frame.get_bytes_per_pixel();
}

rerun::Image color_image(const rs2::video_frame &frame,
                         rerun::ColorModel model) {
  const auto width = static_cast<std::uint32_t>(frame.get_width());
  const auto height = static_cast<std::uint32_t>(frame.get_height());
  // get_data_size() is unreliable for some sources; use the geometry.
  const auto size = static_cast<std::size_t>(frame.get_stride_in_bytes()) *
                    static_cast<std::size_t>(height);
  return rerun::Image(
      rerun::Collection<std::uint8_t>::borrow(frame.get_data(), size),
      {width, height}, model, rerun::ChannelDatatype::U8);
}

rerun::DepthImage depth_image(const rs2::depth_frame &frame) {
  const auto width = static_cast<std::uint32_t>(frame.get_width());
  const auto height = static_cast<std::uint32_t>(frame.get_height());
  // get_units() is meters per unit; Rerun wants units per meter.
  return rerun::DepthImage(static_cast<const std::uint16_t *>(frame.get_data()),
                           {width, height})
      .with_meter(1.0f / frame.get_units());
}

rerun::Boxes2D boxes(const std::vector<Detection> &detections) {
  std::vector<rerun::Vec2D> mins;
  std::vector<rerun::Vec2D> sizes;
  std::vector<rerun::components::Text> labels;
  for (const auto &detection : detections) {
    const auto &box = detection.bbox.rect;
    mins.push_back({static_cast<float>(box.x), static_cast<float>(box.y)});
    sizes.push_back(
        {static_cast<float>(box.width), static_cast<float>(box.height)});
    labels.emplace_back(fmt::format("{:.2f}", detection.bbox.confidence));
  }
  return rerun::Boxes2D::from_mins_and_sizes(mins, sizes)
      .with_labels(std::move(labels));
}

rerun::Points2D keypoints(const std::vector<Detection> &detections) {
  std::vector<rerun::components::Position2D> points;
  for (const auto &detection : detections)
    for (const auto &keypoint : detection.keypoints)
      if (keypoint.confidence >= keypoint_confidence_threshold)
        points.emplace_back(static_cast<float>(keypoint.x),
                            static_cast<float>(keypoint.y));
  return rerun::Points2D(std::move(points));
}

} // namespace viewer

namespace {

using Clock = std::chrono::steady_clock;

double to_ms(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

} // namespace

ViewerSink::ViewerSink(
    std::unique_ptr<rerun_logger::ViewerConnection> connection, Warn warn)
    : viewer_(std::move(connection)), warn_(std::move(warn)) {}

void ViewerSink::calibration(const geometry::Calibration &calibration) {
  // Color is the root frame; depth is placed relative to it.
  viewer_->log_static("color", viewer::pinhole(calibration.intrinsics.color));
  viewer_->log_static("depth",
                      viewer::transform(calibration.extrinsics.depth_to_color),
                      viewer::pinhole(calibration.intrinsics.depth));
}

void ViewerSink::frame(const Frames &frames,
                       const std::vector<Detection> &detections,
                       const LoopTimings &loop_timings) {
  const auto color_frame = frames.get_color_frame();
  const auto depth_frame = frames.get_depth_frame();
  const rs2::frame reference =
      color_frame ? rs2::frame(color_frame) : rs2::frame(depth_frame);
  if (!reference)
    return;

  viewer_->set_time(frame_index_++, viewer::capture_time_ns(reference));
  timings(loop_timings);
  if (color_frame) {
    // Log even when empty, to clear stale boxes.
    viewer_->log("color/detections", viewer::boxes(detections));
    viewer_->log("color/keypoints", viewer::keypoints(detections));
  }

  if (viewer_->image_due()) {
    if (color_frame)
      color(color_frame);
    if (depth_frame)
      depth(depth_frame);
  }
}

void ViewerSink::timings(const LoopTimings &loop_timings) {
  const auto now = Clock::now();
  viewer_->log("health/wait_ms", rerun::Scalars(to_ms(loop_timings.wait)));
  viewer_->log("health/detect_ms", rerun::Scalars(to_ms(loop_timings.detect)));
  if (last_frame_ != Clock::time_point{})
    viewer_->log("health/fps",
                 rerun::Scalars(1000.0 / to_ms(now - last_frame_)));
  last_frame_ = now;
}

void ViewerSink::color(const rs2::video_frame &frame) {
  const auto model = viewer::color_model(frame.get_profile().format());
  if (!model) {
    warn_once(warned_color_format_, "color format is not RGB8, BGR8, RGBA8 "
                                    "or BGRA8; not logging color images");
    return;
  }
  if (!viewer::is_packed(frame)) {
    warn_once(warned_color_stride_, "padded color image rows are not "
                                    "supported; not logging color images");
    return;
  }
  viewer_->log("color", viewer::color_image(frame, *model));
}

void ViewerSink::depth(const rs2::depth_frame &frame) {
  if (frame.get_profile().format() != RS2_FORMAT_Z16) {
    warn_once(warned_depth_format_,
              "depth format is not Z16; not logging depth images");
    return;
  }
  if (!viewer::is_packed(frame)) {
    warn_once(warned_depth_stride_, "padded depth image rows are not "
                                    "supported; not logging depth images");
    return;
  }
  viewer_->log("depth", viewer::depth_image(frame));
}

void ViewerSink::warn_once(bool &warned, std::string_view message) {
  if (!warned)
    warn_(fmt::format("Rerun: {} {}", viewer_->root(), message));
  warned = true;
}
