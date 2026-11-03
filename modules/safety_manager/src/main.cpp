#include "shutdown.hpp"
#include <CLI/CLI.hpp>
#include <csignal>

// int main(int argc, char *argv[]) try {
//   std::signal(SIGINT, handle_shutdown);  // Ctrl+C
//   std::signal(SIGTERM, handle_shutdown); // Docker stop

//   CLI::App app{"RealSense camera node"};