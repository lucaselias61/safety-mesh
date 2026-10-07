// Camera class that persists the information of RealSense cameras.
#pragma once

#include <string>
#include <librealsense2/rs.hpp>

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
    const Intrinsics& get_intrinsics() const noexcept;
    const rs2::device& get_device() const noexcept;

private:
    rs2::device device_;
    std::string serial_;
    Intrinsics intrinsics_;
};
