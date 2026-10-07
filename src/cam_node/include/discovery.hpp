#pragma once

#include <string>
#include <vector>

struct CameraInfo {
    std::string serial;
    std::string name;
};

// Returns connected cameras; SDK failures propagate to the caller.
std::vector<CameraInfo> discover_cameras();
