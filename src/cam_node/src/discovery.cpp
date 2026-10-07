#include "discovery.hpp"

#include <librealsense2/rs.hpp>

std::vector<CameraInfo> discover_cameras() {
    rs2::context context;
    std::vector<CameraInfo> cameras;
    for (const auto& device : context.query_devices()) {
        cameras.push_back({device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER),
                           device.get_info(RS2_CAMERA_INFO_NAME)});
    }
    return cameras;
}
