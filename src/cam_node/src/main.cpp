#include "discovery.hpp"

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc != 2 || std::string_view(argv[1]) != "discovery") {
        std::cerr << "Usage: cam_node discovery\n";
        return 2;
    }

    try {
        const auto cameras = discover_cameras();

        if (cameras.empty()) {
            std::cout << "No RealSense device connected\n";
        }
        for (const auto& camera : cameras) {
            std::cout << camera.serial << '\t' << camera.name << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Camera discovery failed: " << error.what() << '\n';
        return 1;
    }
}
