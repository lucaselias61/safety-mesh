#pragma once

#include <CLI/CLI.hpp>

/// @brief Adds `run`: loads the config, logs camera poses, then polls every
/// robot's PLC and logs it in the world frame until shutdown.
void run_cmd(CLI::App &app);
