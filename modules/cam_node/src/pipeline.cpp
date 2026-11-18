#include "pipeline.hpp"
#include "frames.hpp"

#include <iostream>
#include <librealsense2/rs.hpp>
#include <stdexcept>

namespace {

/// @brief Reads the calibration of the streams the pipeline is running.
/// @return std::nullopt if depth or color is not streaming.
std::optional<geometry::Calibration>
read_calibration(const rs2::pipeline_profile &active) {
  std::optional<rs2::video_stream_profile> depth;
  std::optional<rs2::video_stream_profile> color;
  for (const auto &stream : active.get_streams()) {
    const auto video = stream.as<rs2::video_stream_profile>();
    if (!video)
      continue;
    if (stream.stream_type() == RS2_STREAM_DEPTH)
      depth = video;
    else if (stream.stream_type() == RS2_STREAM_COLOR)
      color = video;
  }
  if (!depth || !color)
    return std::nullopt;
  return geometry::Calibration{
      {depth->get_intrinsics(), color->get_intrinsics()},
      {color->get_extrinsics_to(*depth), depth->get_extrinsics_to(*color)}};
}

/// @brief Drops frames until a whole frameset is in global time.
/// @details Frames queued before enable_global_time() can be in another domain.
/// @throws std::runtime_error if none is within max_framesets.
void wait_for_global_time(rs2::pipeline &pipeline) {
  constexpr int max_framesets = 30;
  for (int i = 0; i < max_framesets; ++i) {
    rs2::frameset frames;
    if (pipeline.try_wait_for_frames(&frames) && in_global_time(frames))
      return;
  }
  throw std::runtime_error("Frames are not in global time");
}

} // namespace

PipelineProfile
PipelineProfile::from_streams(const rs2::device &device,
                              const std::vector<StreamConfig> &streams) {
  PipelineProfile profile;
  for (const auto &stream : streams)
    profile.add(StreamProfile(device, stream));
  return profile;
}

Pipeline::Pipeline(const Camera &camera,
                   const PipelineProfile &pipeline_profile)
    : cam_(camera), pipeline_profile_(pipeline_profile) {
  config_.enable_device(camera.get_serial());

  for (const auto &[id, streams] : pipeline_profile_.get_profiles()) {
    for (const auto &stream : streams) {
      if (!stream.is_video()) {
        config_.enable_stream(stream.stream_type(), -1, stream.format(),
                              stream.fps());
      }
      config_.enable_stream(stream.stream_type(), -1, stream.width(),
                            stream.height(), stream.format(), stream.fps());
      // start();
      // Frames frames = get_frames();
      // if (frames)
    }
  }
}

/// @brief Camera getter
const Camera &Pipeline::camera() const noexcept { return cam_; }

/// @brief Calibration getter
const std::optional<geometry::Calibration> &
Pipeline::calibration() const noexcept {
  return calibration_;
}

/// @brief Start the camera streams with the current configuration
/// @note This will block until the camera is ready to provide frames.
void Pipeline::start() {
  if (!cam_.is_ready()) {
    throw std::runtime_error("Camera is disconnected or in recovery mode");
  }

  const auto active_profile = pipeline_.start(config_);
  try {
    enable_global_time(active_profile.get_device());
    wait_for_global_time(pipeline_);
  } catch (...) {
    pipeline_.stop();
    throw;
  }
  calibration_ = read_calibration(active_profile);
  std::cout << "Enabled streams for camera " << cam_.get_serial() << ":\n"
            << "\n";
  for (const auto &stream : active_profile.get_streams()) {
    std::cout << StreamProfile(stream).to_string();
  }
  std::cout << "\n";
  std::cout.flush();
}

/// @brief Stop the camera streams
/// @note This will block until the camera has completely stopped providing
/// frames.
void Pipeline::stop() {
  pipeline_.stop();
  calibration_.reset();
}

/// @brief Get the latest frames from the camera streams
/// @note This will not block indefinitely. It will return immediately if no
/// frames are available.
/// @return The latest set of frames from the active camera streams (color |
/// depth | accel | gyro).
/// @throws std::runtime_error if the camera device is not connected.
Frames Pipeline::get_frames() {
  rs2::frameset frameset;
  // here we use try_wait_for_frames instead of wait_for_frames to avoid
  // blocking indefinitely
  pipeline_.try_wait_for_frames(&frameset);
  return Frames(frameset);
}
