#pragma once

#include "camera.hpp"
#include "worker.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>


inline void list_cmd(CLI::App& app) {
    app.add_subcommand("list", "List connected RealSense cameras")->callback([]() {
        const auto cameras = discover_cameras();
        if (cameras.empty()) {
            std::cout << "No RealSense device connected\n";
        }
        for (const auto& camera : cameras) {
            std::cout << camera.get_serial() << '\t' << camera.get_name() << '\n';
        }
    });
}


inline void run_cmd(CLI::App& app) {
    // The callback owns these objects beyond command registration.
    auto serial = std::make_shared<std::string>();
    auto* run_command = app.add_subcommand("run", "Run a connected camera");
    run_command->add_option("--serial", *serial, "Run a camera by serial number")
    ->required()
    ->check([](const std::string& value) {return value.empty() ? "Serial number cannot be empty" : "";});

    run_command->callback([serial] {
        for (const auto& camera : discover_cameras()) {
            if (camera.get_serial() == *serial) {
                Worker worker(camera);
                worker.run();
                return;
            }
        }
        throw std::runtime_error("No RealSense device connected with serial " + *serial);
    });
}



