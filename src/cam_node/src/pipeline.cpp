#include "pipeline.hpp"

#include <librealsense2/rs.hpp>


Frames::Frames(const rs2::frameset& f) : frames_(f) {}

rs2::video_frame Frames::get_color_frame() const {return frames_.get_color_frame();}

rs2::depth_frame Frames::get_depth_frame() const {return frames_.get_depth_frame();}

rs2::motion_frame Frames::get_accel_frame() const {return frames_.first_or_default(RS2_STREAM_ACCEL).as<rs2::motion_frame>();}

rs2::motion_frame Frames::get_gyro_frame() const {return frames_.first_or_default(RS2_STREAM_GYRO).as<rs2::motion_frame>();}


Pipeline::Pipeline(const Camera& camera) : cam_(camera) {
    config_.enable_device(camera.get_serial());
    config_.enable_stream(RS2_STREAM_COLOR);
    config_.enable_stream(RS2_STREAM_DEPTH);
    config_.enable_stream(RS2_STREAM_ACCEL);
    config_.enable_stream(RS2_STREAM_GYRO);
}

/// @brief Camera getter
const Camera& Pipeline::camera() const noexcept {
    return cam_;
}

/// @brief Start the camera streams with the current configuration
/// @note This will block until the camera is ready to provide frames.
void Pipeline::start() {
    pipeline_.start(config_);
}

/// @brief Stop the camera streams
/// @note This will block until the camera has completely stopped providing frames.
void Pipeline::stop() {
    pipeline_.stop();
}

/// @brief Get the latest frames from the camera streams
Frames Pipeline::get_frames() {
    rs2::frameset frameset = pipeline_.wait_for_frames();
    Frames frames(frameset);
    return frames;
}

