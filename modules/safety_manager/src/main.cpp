#include "shutdown.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>

#include <csignal>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char *argv[]) try {
  std::signal(SIGINT, handle_shutdown);  // Ctrl+C
  std::signal(SIGTERM, handle_shutdown); // Docker stop

  CLI::App app{"Safety Manager", "safety_manager"};
  app.set_version_flag("--version", SAFETY_MANAGER_VERSION);

  try {
    // CLI11 expects the arguments in reverse order, without the program name.
    std::vector<std::string> args(std::make_reverse_iterator(argv + argc),
                                  std::make_reverse_iterator(argv + 1));
    app.parse(std::move(args));
  } catch (const CLI::ParseError &error) {
    return app.exit(error);
  }
  return 0;
} catch (const std::exception &error) {
  std::cerr << "(Safety Manager error): " << error.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "(Safety Manager error): Unknown exception\n";
  return 1;
}
