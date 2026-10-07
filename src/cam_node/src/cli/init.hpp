#pragma once

#include <camera.hpp>
#include <CLI/CLI.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <string>


struct InitOptions {
    bool all = false;
    std::string serial;
};


inline void init(bool all, const std::string& serial) {
    rs2::context context;
    std::vector<Camera> cameras;
    for (const auto& device : context.query_devices()) {
        if (all || serial == device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)) {
            cameras.emplace_back(device);
            if (!all) break;
        }
    }
    if (cameras.empty()) {
        throw std::runtime_error(all ? "No RealSense device connected"
                                    : "No RealSense device connected with serial " + serial);
    }
    for (const auto& camera : cameras) {
        std::cout << "Initialized camera " << camera.get_serial() << '\n';
    }
}

inline void init_cmd(CLI::App& app) {
    // The callback owns these objects beyond command registration.
    auto options = std::make_shared<InitOptions>();
    auto cameras = std::make_shared<std::vector<Camera>>();
    auto* init_command = app.add_subcommand("init", "Initialize connected cameras");
    auto* selection = init_command->add_option_group("Camera selection");
    selection->require_option(1);
    selection->add_flag("--all", options->all, "Initialize all connected cameras");
    selection->add_option("--serial", options->serial, "Initialize a camera by serial number")
        ->check([](const std::string& value) {
            return value.empty() ? "Serial number cannot be empty" : "";
        });
    init_command->callback([options, cameras] {
        init(options->all, options->serial);
    });
}

