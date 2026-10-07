#pragma once

#include <discovery.hpp>
#include <iostream>
#include <CLI/CLI.hpp>


inline void discovery() {
    const auto cameras = discover_cameras();

    if (cameras.empty()) {
        std::cout << "No RealSense device connected\n";
    }
    for (const auto& camera : cameras) {
        std::cout << camera.serial << '\t' << camera.name << '\n';
    }
}


inline void discovery_cmd(CLI::App& app) {
    app.add_subcommand("discovery", "List connected RealSense cameras")->callback(discovery);
}

