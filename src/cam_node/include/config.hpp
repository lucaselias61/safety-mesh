#pragma once

#include "stream.hpp"

#include <string>
#include <vector>

struct CameraConfig {
    std::string serial;
    bool enabled;
    std::vector<StreamConfig> streams;
};

// Throws std::runtime_error if the file cannot be read or is invalid.
CameraConfig load_camera_config(const std::string& path);
