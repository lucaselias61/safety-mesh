#pragma once

#include <librealsense2/rs.hpp>
#include <string>
#include <vector>


struct StreamConfig {

    std::string name;
    int width;
    int height;
    int fps;
    std::string format;
};



class StreamProfile : rs2::stream_profile {
public:
    /// @brief Constructs a StreamProfile from the given device and stream configuration if supported.
    /// @param device The RealSense device.
    /// @param config The desired stream configuration.
    /// @throws std::runtime_error if no matching stream profile is found.
    explicit StreamProfile(const rs2::device& device, const StreamConfig& config);
    explicit StreamProfile(const rs2::stream_profile& profile) : rs2::stream_profile(profile) {}

    int id() const;
    std::string name() const;
    rs2_stream stream_type() const;
    rs2_format format() const;
    int width() const;
    int height() const;
    int fps() const;

    bool is_video() const;
    std::string to_string() const;
};

