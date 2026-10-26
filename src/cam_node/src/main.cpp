#include "camera.hpp"
#include "cli.hpp"
#include "shutdown.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  std::signal(SIGINT, handle_shutdown);  // Ctrl+C
  std::signal(SIGTERM, handle_shutdown); // Docker stop

  CLI::App app{"RealSense camera node"};
  app.require_subcommand(1);

  list_cmd(app);
  run_cmd(app);
  streams_cmd(app);
  view_cmd(app);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &error) {
    return app.exit(error);
  } catch (const rs2::error &error) {
    std::cerr << "(RealSense error): " << error.get_failed_function() << "("
              << error.get_failed_args() << "): " << error.what() << '\n';
    return 1;
  } catch (const std::exception &error) {
    std::cerr << "(Camera Node error): " << error.what() << '\n';
    return 1;
  }
  return 0;
}
