// Camera class that persists the information of RealSense cameras.
#pragma once
#include "stream.hpp"
#include <librealsense2/rs.hpp>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

/** \brief Extension of the 'device' class on the rs2 SDK. Stores information
 * about a RealSense camera. */
class Camera {
public:
  explicit Camera(const rs2::device &device);

  const std::string &get_serial() const noexcept;
  const std::string &get_name() const noexcept;
  const rs2::device &get_device() const noexcept;

  std::vector<StreamProfile> get_supported_stream_profiles() const;
  std::vector<StreamProfile> get_active_stream_profiles() const;

  /// @brief Checks if the camera device is still connected.
  bool is_ready() const;
  void reset();

private:
  std::string serial_;
  std::string name_;
  rs2::device device_;
};

/// Result of one device scan.
struct Discovery {
  std::vector<Camera> cameras;
  // One SDK message per device that could not be opened.
  std::vector<std::string> unavailable;
};

// Returns plugged-in cameras.
// - A device that cannot be opened is skipped and listed in unavailable.
Discovery discover_cameras();

// Returns the connected camera with this serial.
// Throws std::runtime_error if it is not found. The message names devices that
// could not be opened, since the camera may be one of them.
Camera require_camera(const std::string &serial);
