#include "camera.hpp"
#include "cli.hpp"
#include "stream.hpp"
#include "utils.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

void streams(const std::shared_ptr<std::string> &serial,
             const std::shared_ptr<std::string> &key,
             const std::shared_ptr<std::string> &filter) {
  for (const auto &camera : discover_cameras()) {
    if (camera.get_serial() == *serial) {
      const auto streams = sort_stream_profiles_by_key(
          filter_stream_profiles(camera.get_supported_stream_profiles(),
                                 *filter),
          *key);

      if (streams.empty()) {
        std::cout << "No supported stream profiles found.\n";
        return;
      }

      for (const auto &stream : streams) {
        std::cout << stream.to_string();
      }
      return;
    } // Exit the callback after processing the matching camera
  }
  throw std::runtime_error("No RealSense device connected with serial " +
                           *serial);
}

} // namespace

void streams_cmd(CLI::App &app) {
  auto serial = std::make_shared<std::string>();
  auto *streams_command = app.add_subcommand(
      "streams", "List supported stream profiles for a connected camera");
  streams_command
      ->add_option("--serial", *serial,
                   "Specify the serial number of the camera")
      ->required()
      ->check([](const std::string &value) {
        return value.empty() ? "Serial number cannot be empty" : "";
      });
  auto key = std::make_shared<std::string>("all");
  streams_command->add_option(
      "--sort", *key,
      "Sort within each stream ID (all, fps, width, height, resolution)");
  auto filter = std::make_shared<std::string>("all");
  streams_command->add_option(
      "--filter", *filter,
      "Filter stream profiles by type (all, color, depth, infrared)");

  streams_command->callback(
      [serial, key, filter] { streams(serial, key, filter); });
}
