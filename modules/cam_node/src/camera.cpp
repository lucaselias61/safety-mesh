#include "camera.hpp"
#include "pipeline.hpp"
#include "stream.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <librealsense2/rs.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

std::vector<StreamProfile>
_get_supported_stream_profiles(const rs2::device &device) {
  std::vector<StreamProfile> streams;
  for (const auto &sensor : device.query_sensors()) {
    for (const auto &profile : sensor.get_stream_profiles()) {
      streams.push_back(StreamProfile(profile));
    }
  }
  return streams;
}

std::vector<StreamProfile>
_get_active_stream_profiles(const rs2::device &device) {
  std::vector<StreamProfile> streams;
  for (const auto &sensor : device.query_sensors()) {
    for (const auto &profile : sensor.get_active_streams()) {
      streams.push_back(StreamProfile(profile));
    }
  }
  return streams;
}

} // namespace

/// @brief Constructs a Camera object with the given device.
/// @param device The RealSense device used to construct the Camera object.
Camera::Camera(const rs2::device &device) {
  serial_ = device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
  name_ = device.get_info(RS2_CAMERA_INFO_NAME);
  device_ = device;
}

/// @brief serial getter
const std::string &Camera::get_serial() const noexcept { return serial_; }

/// @brief name getter
const std::string &Camera::get_name() const noexcept { return name_; }

/// @brief RealsSense device getter
const rs2::device &Camera::get_device() const noexcept { return device_; }

/// @brief supported streams getter
/// @return The list of supported stream profiles for the camera.
std::vector<StreamProfile> Camera::get_supported_stream_profiles() const {
  return _get_supported_stream_profiles(device_);
}

/// @brief active streams getter
/// @return The list of active stream profiles for the camera.
std::vector<StreamProfile> Camera::get_active_stream_profiles() const {
  return _get_active_stream_profiles(device_);
}

/// @brief Checks if the camera device is still connected.
bool Camera::is_ready() const {
  if (!device_ || !device_.is_connected()) {
    return false;
  }
  auto device = device_;
  return !device.is_in_recovery_mode();
}

/// @brief Resets the camera device.
void Camera::reset() { device_.hardware_reset(); }

Discovery discover_cameras() {
  rs2::context context;
  const auto devices = context.query_devices();
  Discovery discovery;
  // Devices opened on access. Busy must not hide the others.
  for (std::uint32_t i = 0; i < devices.size(); ++i) {
    try {
      discovery.cameras.push_back(Camera(devices[i]));
    } catch (const rs2::error &error) {
      discovery.unavailable.push_back("device " + std::to_string(i) + ": " +
                                      error.what());
    }
  }
  return discovery;
}

Camera require_camera(const std::string &serial) {
  const auto discovery = discover_cameras();
  for (const auto &camera : discovery.cameras)
    if (camera.get_serial() == serial)
      return camera;
  std::string message = "No RealSense device connected with serial " + serial;
  if (!discovery.unavailable.empty()) {
    message += ". Devices that could not be connected:";
    for (const auto &device : discovery.unavailable)
      message += "\n  " + device;
  }
  throw std::runtime_error(message);
}
