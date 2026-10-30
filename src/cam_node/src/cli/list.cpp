#include "camera.hpp"
#include "cli.hpp"

#include <CLI/CLI.hpp>
#include <iostream>

namespace {

void list() {
  const auto cameras = discover_cameras();
  if (cameras.empty()) {
    std::cout << "No RealSense device connected\n";
  }
  for (const auto &camera : cameras) {
    std::cout << camera.get_serial() << '\t' << camera.get_name() << '\n';
  }
}

} // namespace

void list_cmd(CLI::App &app) {
  app.add_subcommand("list", "List connected RealSense cameras")
      ->callback([]() { list(); });
}
