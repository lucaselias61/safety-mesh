#include "discovery.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>

void discovery() {
    const auto cameras = discover_cameras();

    if (cameras.empty()) {
        std::cout << "No RealSense device connected\n";
    }
    for (const auto& camera : cameras) {
        std::cout << camera.serial << '\t' << camera.name << '\n';
    }
}

void init() {
    // Initialization code for the camera node can be added here.
}

int main(int argc, char* argv[]) {
    CLI::App app{"RealSense camera node"};
    app.require_subcommand(1);

    app.add_subcommand("discovery", "List connected RealSense cameras")->callback(discovery);
    app.add_subcommand("init", "Initialize the camera node")->callback(init);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
        return app.exit(error);
    } catch (const std::exception& error) {
        std::cerr << "Camera discovery failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
