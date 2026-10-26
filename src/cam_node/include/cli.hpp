#pragma once

#include <CLI/CLI.hpp>

/**
 * \brief lists the connected RealSense cameras.
 * \param app The CLI app to which the list command will be added.
 * \note This command will print the serial number and name of each connected
 * RealSense camera.
 */
void list_cmd(CLI::App &app);

/**
 * \brief Runs the detection pipeline for a connected camera by its serial
 * number.
 * \param app The CLI app to which the run command will be added.
 */
void run_cmd(CLI::App &app);

/**
 * \brief Lists the available streams for the connected RealSense cameras.
 * \param app The CLI app to which the streams command will be added.
 */
void streams_cmd(CLI::App &app);

/// Displays color and a depth heatmap side by side in an OpenCV window.
void view_cmd(CLI::App &app);
