// CameraStream class manages camera streams and configuration.
#pragma once

#include "camera.hpp"
#include "stream.hpp"

#include <filesystem>
#include <string>
#include <map>
#include <librealsense2/rs.hpp>


class PipelineProfile {
public:
    void add(const StreamProfile& profile) {
        profiles_[profile.id()].push_back(profile);
    }

    const std::map<int, std::vector<StreamProfile>>& get_profiles() const { return profiles_; }
    std::string to_string() const;

private:
    std::map<int, std::vector<StreamProfile>> profiles_;
};


/**
 * \brief Represents a set of frames captured from the camera streams.
 * \note This class provides access to the individual frames (color, depth, accelerometer, gyroscope) captured.
 * All frames are synchronized to the same timestamp.
 */
class Frames {
public:
    explicit Frames(const rs2::frameset& f);

    /// @brief Checks if the frameset contains valid frames.
    bool operator!() const {
        return !static_cast<bool>(frames_);
    }

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
    explicit Pipeline(const Camera& camera, const PipelineProfile& pipeline_profile);

    void start();
    void stop();

    Frames get_frames();
    const Camera& camera() const noexcept;

private:
    Camera cam_;
    rs2::pipeline pipeline_;
    rs2::config config_;
    PipelineProfile pipeline_profile_;
};
