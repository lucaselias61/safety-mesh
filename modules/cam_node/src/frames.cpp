#include "frames.hpp"

#include <stdexcept>
#include <string>

Frames::Frames(const rs2::frameset &f) : frames_(f) {}

rs2::video_frame Frames::get_color_frame() const {
  return frames_.get_color_frame();
}
rs2::depth_frame Frames::get_depth_frame() const {
  return frames_.get_depth_frame();
}
rs2::motion_frame Frames::get_accel_frame() const {
  return frames_.first_or_default(RS2_STREAM_ACCEL).as<rs2::motion_frame>();
}
rs2::motion_frame Frames::get_gyro_frame() const {
  return frames_.first_or_default(RS2_STREAM_GYRO).as<rs2::motion_frame>();
}

bool in_global_time(const rs2::frameset &frames) {
  for (const auto &frame : frames)
    if (frame.get_frame_timestamp_domain() != RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME)
      return false;
  return true;
}

std::chrono::system_clock::time_point capture_time(const rs2::frame &frame) {
  if (frame.get_frame_timestamp_domain() != RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME)
    throw std::runtime_error("Frame is not in global time");
  // get_timestamp() is milliseconds since the epoch.
  const std::chrono::duration<double, std::milli> since_epoch(
      frame.get_timestamp());
  return std::chrono::system_clock::time_point(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          since_epoch));
}

std::chrono::system_clock::duration frame_age(const rs2::frame &frame) {
  return std::chrono::system_clock::now() - capture_time(frame);
}

void enable_global_time(const rs2::device &device) {
  for (auto &sensor : device.query_sensors()) {
    if (sensor.get_active_streams().empty())
      continue;
    if (!sensor.supports(RS2_OPTION_GLOBAL_TIME_ENABLED)) {
      const std::string name = sensor.supports(RS2_CAMERA_INFO_NAME)
                                   ? sensor.get_info(RS2_CAMERA_INFO_NAME)
                                   : "A sensor";
      throw std::runtime_error(name + " does not support global time");
    }
    sensor.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, 1.f);
  }
}
