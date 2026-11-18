#pragma once

// Frames from the camera: the frameset cam_node works on, and frame timing.

#include <librealsense2/rs.hpp>

#include <chrono>

/**
 * \brief Represents a set of frames captured from the camera streams.
 * \note This class provides access to the individual frames (color, depth,
 * accelerometer, gyroscope) captured. All frames are synchronized to the same
 * timestamp.
 */
class Frames {
public:
  explicit Frames(const rs2::frameset &f);

  /// @brief Checks if the frameset contains valid frames.
  bool operator!() const { return !static_cast<bool>(frames_); }

  rs2::video_frame get_color_frame() const;
  rs2::depth_frame get_depth_frame() const;
  rs2::motion_frame get_accel_frame() const;
  rs2::motion_frame get_gyro_frame() const;

  /// Every frame of the set, for checks across all streams.
  const rs2::frameset &frameset() const noexcept { return frames_; }

private:
  rs2::frameset frames_;
};

// Global time is mandatory. Frames in any other domain are errors.
// - GLOBAL_TIME: camera clock mapped to the host clock, close to exposure.

/// @return True if every frame of the set is in GLOBAL_TIME.
bool in_global_time(const rs2::frameset &frames);

/// @return Capture time on the host clock.
/// @throws std::runtime_error if the frame isn't in GLOBAL_TIME.
std::chrono::system_clock::time_point capture_time(const rs2::frame &frame);

/// @return Time from capture to now. Can be slightly negative with clock
/// mapping jitter.
/// @throws std::runtime_error if the frame isn't in GLOBAL_TIME.
std::chrono::system_clock::duration frame_age(const rs2::frame &frame);

/// @brief Turns on global time for every streaming sensor of the device.
/// @details Call after the pipeline starts, on the pipeline's own device.
/// @throws std::runtime_error if a streaming sensor doesn't support it.
void enable_global_time(const rs2::device &device);
