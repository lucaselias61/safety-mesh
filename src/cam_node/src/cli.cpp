#include "cli.hpp"
#include "camera.hpp"
#include "commands.hpp"
#include "engines/opencv_dnn_engine.hpp"
#include "shutdown.hpp"
#include "utils.hpp"
#include "worker.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

void list_cmd(CLI::App &app) {
  app.add_subcommand("list", "List connected RealSense cameras")
      ->callback([]() { list(); });
}

void view_cmd(CLI::App &app) {
  auto serial = std::make_shared<std::string>();
  auto *command = app.add_subcommand(
      "view", "Preview color and depth heatmap side by side");
  command->add_option("--serial", *serial, "Camera serial number")
      ->required()
      ->check([](const std::string &value) {
        return value.empty() ? "Serial number cannot be empty" : "";
      });

  command->callback([serial] { view(serial); });
}

void run_cmd(CLI::App &app) {
  // The callback owns these objects beyond command registration.
  auto config = std::make_shared<std::string>();
  auto *run_command = app.add_subcommand("run", "Run a connected camera");
  run_command
      ->add_option("--config", *config, "Path to the camera configuration file")
      ->required()
      ->check(CLI::ExistingFile);

  run_command->callback([config] { run(config); });
}

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
      "--sort", *key, "Sort within each stream ID (all, fps, width, height, resolution)");
  auto filter = std::make_shared<std::string>("all");
  streams_command->add_option(
      "--filter", *filter,
      "Filter stream profiles by type (all, color, depth, infrared)");

  streams_command->callback(
      [serial, key, filter] { streams(serial, key, filter); });
}
