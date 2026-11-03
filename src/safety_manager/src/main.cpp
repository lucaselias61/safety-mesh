#include <csignal>
#include <CLI/CLI.hpp>
#include "shutdown.hpp"

int main(int argc, char *argv[]) try {
  std::signal(SIGINT, handle_shutdown);  // Ctrl+C
  std::signal(SIGTERM, handle_shutdown); // Docker stop

  CLI::App app{"RealSense camera node"}