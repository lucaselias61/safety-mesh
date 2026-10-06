#include <librealsense2/rs.hpp>

#include <exception>
#include <iostream>

int main() {
    try {
        rs2::context context;
        const auto devices = context.query_devices();

        if (devices.size() == 0) {
            std::cout << "No RealSense device connected\n";
            return 0;
        }

        for (const auto& device : devices) {
            std::cout << device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                      << '\t' << device.get_info(RS2_CAMERA_INFO_NAME) << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Camera discovery failed: " << error.what() << '\n';
        return 1;
    }
}
