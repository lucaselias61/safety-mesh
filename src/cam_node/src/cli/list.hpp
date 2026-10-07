#pragma once

#include <discovery.hpp>

#include <iostream>
#include <CLI/CLI.hpp>


inline void list() {
    const auto cameras = discover_cameras();

    if (cameras.empty()) {
        std::cout << "No RealSense device connected\n";
    }
    for (const auto& camera : cameras) {
        std::cout << camera.serial << '\t' << camera.name << '\n';
    }
}


inline void list_cmd(CLI::App& app) {
    app.add_subcommand("list", "List connected RealSense cameras")->callback(list);
}

