#include "cli.hpp"
#include "camera.hpp"
#include "worker.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>


void list_cmd(CLI::App& app) {
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


void run_cmd(CLI::App& app) {
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


void streams_cmd(CLI::App& app) {
    auto serial = std::make_shared<std::string>();
    auto* streams_command = app.add_subcommand("streams", "List available streams for a connected camera");
    streams_command->add_option("--serial", *serial, "Specify the serial number of the camera")
    ->required()
    ->check([](const std::string& value) {return value.empty() ? "Serial number cannot be empty" : "";});
    auto supported = std::make_shared<bool>(false);
    streams_command->add_flag("--supported", *supported, "List all supported streams for the camera");

    streams_command->callback([serial, supported] {
        for (const auto& camera : discover_cameras()) {
             if (camera.get_serial() == *serial) {
                const auto streams = *supported
                    ? camera.get_supported_stream_profiles()
                    : camera.get_active_stream_profiles();

                std::cout << streams.to_string();
                return;
            }// Exit the callback after processing the matching camera
        }
        throw std::runtime_error("No RealSense device connected with serial " + *serial);
    });
}
