#pragma once

#include <librealsense2/rs.hpp>

#include <chrono>

// Global time is mandatory. Frames in any other domain are errors.
// - GLOBAL_TIME: camera clock mapped to the host clock, close to exposure.

/// @return True if every frame of the set is in GLOBAL_TIME.
bool in_global_time(const rs2::frameset &frames);

/// @return Capture time on the host clock.
/// @throws std::runtime_error if the frame isn't in GLOBAL_TIME.
std::chrono::system_clock::time_point capture_time(const rs2::frame &frame);

/// @return Time from capture to now. Can be slightly negative with clock
/// mapping jitter.
/// @throws std::runtime_error if the frame isn't in GLOBAL_TIME.
std::chrono::system_clock::duration frame_age(const rs2::frame &frame);

/// @brief Turns on global time for every streaming sensor of the device.
/// @details Call after the pipeline starts, on the pipeline's own device.
/// @throws std::runtime_error if a streaming sensor doesn't support it.
void enable_global_time(const rs2::device &device);
