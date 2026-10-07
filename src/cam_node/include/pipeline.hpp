// CameraStream class manages camera streams and configuration.
#pragma once

#include <filesystem>
#include <string>

#include "camera.hpp"


class Frames {
public:
    explicit Frames(const rs2::frameset& f);

    rs2::video_frame get_color_frame() const;
    rs2::depth_frame get_depth_frame() const;
    rs2::motion_frame get_accel_frame() const;
    rs2::motion_frame get_gyro_frame() const;

private:
    rs2::frameset frames_;
};

/** \brief This class manages one camera's pipeline and configuration.
 * \note It wraps the RealSense pipeline and provides methods to control and access the necessary streams.
 */
class Pipeline {
public:
    explicit Pipeline(const Camera& camera);

    void start();
    void stop();

    Frames get_frames();
    const Camera& camera() const noexcept;

private:
    Camera cam_;
    rs2::pipeline pipeline_;
    rs2::config config_;
};
