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

struct StreamsOptions {
  std::string serial;
  std::string sort_key = "all";
  std::string filter = "all";
};

void streams(const StreamsOptions &options) {
  for (const auto &camera : discover_cameras()) {
    if (camera.get_serial() == options.serial) {
      const auto streams = sort_stream_profiles_by_key(
          filter_stream_profiles(camera.get_supported_stream_profiles(),
                                 options.filter),
          options.sort_key);

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
                           options.serial);
}

} // namespace

void streams_cmd(CLI::App &app) {
  auto options = std::make_shared<StreamsOptions>();
  auto *streams_command = app.add_subcommand(
      "streams", "List supported stream profiles for a connected camera");
  streams_command
      ->add_option("--serial", options->serial,
                   "Specify the serial number of the camera")
      ->required()
      ->check([](const std::string &value) {
        return value.empty() ? "Serial number cannot be empty" : "";
      });
  streams_command->add_option(
      "--sort", options->sort_key,
      "Sort within each stream ID (all, fps, width, height, resolution)");
  streams_command->add_option(
      "--filter", options->filter,
      "Filter stream profiles by type (all, color, depth, infrared)");

  streams_command->callback([options] { streams(*options); });
}
