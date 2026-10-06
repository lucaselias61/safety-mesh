#include "../include/camera.hpp"

#include <librealsense2/rs.hpp>

#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

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

} // namespace

/// @brief Constructs a Camera object with the given device.
/// @param device
Camera::Camera(rs2::device device) noexcept {
    device_ = device;
    intrinsics_ = read_intrinsics(device);
    serial_ = device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
}

/// @brief serial getter
const std::string& Camera::get_serial() const noexcept {
    return serial_;
}

/// @brief intrinsics getter
const Intrinsics& Camera::get_intrinsics() const noexcept {
    return intrinsics_;
}

/// @brief RealsSense device getter
const rs2::device& Camera::get_device() const noexcept {
    return device_;
}
