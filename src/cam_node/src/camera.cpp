#include "camera.hpp"
#include "pipeline.hpp"
#include "stream.hpp"

#include <librealsense2/rs.hpp>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <algorithm>


namespace {

/// @brief Retrieves the intrinsics for a video or depth stream from the given device.
/// @param device The RealSense device.
/// @param stream The stream type.
/// @return Optional intrinsics if available, or std::nullopt if not.
std::optional<rs2_intrinsics> get_intrinsic_info(const rs2::device& device, rs2_stream stream) {
    std::optional<rs2_intrinsics> fallback;

    for (const auto& sensor : device.query_sensors()) {
        for (const auto& profile : sensor.get_stream_profiles()) {

            if (profile.stream_type() != stream) {continue;}

            const auto video_profile = profile.as<rs2::video_stream_profile>();
            if (!video_profile) {continue;}

            const auto intrinsics = video_profile.get_intrinsics();
            if (profile.is_default()) {return intrinsics;}

            if (!fallback) {fallback = intrinsics;}
        }
    }
    return fallback;
}

Intrinsics read_intrinsics(const rs2::device& device) {
    const auto depth = get_intrinsic_info(device, RS2_STREAM_DEPTH);
    const auto color = get_intrinsic_info(device, RS2_STREAM_COLOR);

    if (!depth || !color) {throw std::runtime_error("The camera does not expose depth and color intrinsics");}

    return Intrinsics{*depth, *color};
}

std::vector<StreamProfile> _get_supported_stream_profiles(const rs2::device& device) {
    std::vector<StreamProfile> streams;
    for (const auto& sensor : device.query_sensors()) {
        for (const auto& profile : sensor.get_stream_profiles()) {
            streams.push_back(StreamProfile(profile));
        }
    }
    return streams;
}

std::vector<StreamProfile> _get_active_stream_profiles(const rs2::device& device) {
    std::vector<StreamProfile> streams;
    for (const auto& sensor : device.query_sensors()) {
        for (const auto& profile : sensor.get_active_streams()) {
            streams.push_back(StreamProfile(profile));
        }
    }
    return streams;  
}

} // namespace

/// @brief Constructs a Camera object with the given device.
/// @param device The RealSense device used to construct the Camera object.
Camera::Camera(rs2::device device) {
    serial_ = device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    name_ = device.get_info(RS2_CAMERA_INFO_NAME);
    intrinsics_ = read_intrinsics(device);
    device_ = device;
}

/// @brief serial getter
const std::string& Camera::get_serial() const noexcept {return serial_;}

/// @brief name getter
const std::string& Camera::get_name() const noexcept {return name_;}

/// @brief intrinsics getter
const Intrinsics& Camera::get_intrinsics() const noexcept {return intrinsics_;}

/// @brief RealsSense device getter
const rs2::device& Camera::get_device() const noexcept {return device_;}

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
    if (!device_ || !device_.is_connected()) {return false;}
    auto device = device_;
    return !device.is_in_recovery_mode();
}


std::vector<Camera> discover_cameras() {
    rs2::context context;
    std::vector<Camera> cameras;
    for (const auto& device : context.query_devices()) {
        cameras.push_back(Camera(device));
    }
    return cameras;
}


