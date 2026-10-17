// Camera class that persists the information of RealSense cameras.
#pragma once
#include <map>
#include <string>
#include <vector>
#include <set>
#include <librealsense2/rs.hpp>
#include "stream.hpp"


/// Depth and color calibration parameters for a camera.
struct Intrinsics {
    
    rs2_intrinsics depth;
    rs2_intrinsics color;
};

/** \brief Extension of the 'device' class on the rs2 SDK. Stores information about a RealSense camera. */
class Camera {
public:
    explicit Camera(rs2::device device);

    const std::string& get_serial() const noexcept;
    const std::string& get_name() const noexcept;
    const Intrinsics& get_intrinsics() const noexcept;
    const rs2::device& get_device() const noexcept;
    
    std::vector<StreamProfile> get_supported_stream_profiles() const;
    std::vector<StreamProfile> get_active_stream_profiles() const;

    /// @brief Checks if the camera device is still connected.
    bool is_ready() const;

private:
    std::string serial_;
    std::string name_;
    Intrinsics intrinsics_;
    rs2::device device_;
};

// Returns connected cameras; SDK failures propagate to the caller.
std::vector<Camera> discover_cameras();
