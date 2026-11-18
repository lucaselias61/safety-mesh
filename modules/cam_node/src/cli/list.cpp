#include "camera.hpp"
#include "cli.hpp"

#include <CLI/CLI.hpp>
#include <iostream>

namespace {

// Cameras go to stdout as "serial<TAB>name". docker/entrypoint.py parses them.
// Devices that could not be opened go to stderr.
void list() {
  const auto discovery = discover_cameras();
  if (discovery.cameras.empty() && discovery.unavailable.empty()) {
    std::cout << "No RealSense device connected\n";
  }
  for (const auto &camera : discovery.cameras) {
    std::cout << camera.get_serial() << '\t' << camera.get_name() << '\n';
  }
  for (const auto &device : discovery.unavailable) {
    std::cerr << "Unavailable " << device << '\n';
  }
}

} // namespace

void list_cmd(CLI::App &app) {
  app.add_subcommand("list", "List connected RealSense cameras")
      ->callback([]() { list(); });
}
