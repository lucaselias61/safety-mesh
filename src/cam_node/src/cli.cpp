#include "cli.hpp"
#include "config.hpp"
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
    auto config = std::make_shared<std::string>();
    auto* run_command = app.add_subcommand("run", "Run a connected camera");
    run_command->add_option("--config", *config, "Path to the camera configuration file")
    ->required()
    ->check(CLI::ExistingFile);

    run_command->callback([config] {
        const auto camera_config = load_camera_config(*config);
        if (!camera_config.enabled) {
            return;
        }
        if (camera_config.streams.empty()) {
            throw std::runtime_error("No streams enabled in camera configuration");
        }
        for (const auto& camera : discover_cameras()) {
            if (camera.get_serial() == camera_config.serial) {
                PipelineProfile profile;
                for (const auto& stream : camera_config.streams) {
                    profile.add(StreamProfile(camera.get_device(), stream));
                }
                Worker worker(camera, profile);
                worker.run();
                return;
            }
        }
        throw std::runtime_error("No RealSense device connected with serial " + camera_config.serial);
    });
}


void streams_cmd(CLI::App& app) {
    auto serial = std::make_shared<std::string>();
    auto* streams_command = app.add_subcommand("streams", "List supported stream profiles for a connected camera");
    streams_command->add_option("--serial", *serial, "Specify the serial number of the camera")
    ->required()
    ->check([](const std::string& value) {return value.empty() ? "Serial number cannot be empty" : "";});
    auto key = std::make_shared<std::string>("all");
    streams_command->add_option("--key", *key, "Filter and sort stream profiles by key (all, color, depth, infrared, fps, width, height)"); 

    streams_command->callback([serial, key] {
        for (const auto& camera : discover_cameras()) {
             if (camera.get_serial() == *serial) {
                const auto streams = sort_n_filter_stream_profiles_by_key(
                    camera.get_supported_stream_profiles(), *key);

                if (streams.empty()) {
                    std::cout << "No supported stream profiles found.\n";
                    return;
                }

                for (const auto& stream : streams) {
                    std::cout << stream.to_string();
                }
                return;
            }// Exit the callback after processing the matching camera
        }
        throw std::runtime_error("No RealSense device connected with serial " + *serial);
    });
}
