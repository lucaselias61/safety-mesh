#include "stream.hpp"

#include <algorithm>
#include <cctype>
#include <librealsense2/rs.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

StreamProfile find_stream_profile(const rs2::device &device,
                                  const StreamConfig &config) {
  for (const auto &sensor : device.query_sensors()) {
    for (const auto &profile : sensor.get_stream_profiles()) {
      if (config.name != rs2_stream_to_string(profile.stream_type()) ||
          config.format != rs2_format_to_string(profile.format()) ||
          config.fps != profile.fps()) {
        continue;
      }
      const auto video = profile.as<rs2::video_stream_profile>();
      if (video &&
          (video.width() != config.width || video.height() != config.height)) {
        continue;
      }
      return StreamProfile(profile);
    }
  }
  throw std::runtime_error(
      "No matching stream profile found for the given configuration");
}
} // namespace

StreamProfile::StreamProfile(const rs2::device &device,
                             const StreamConfig &config) {
  *this = find_stream_profile(device, config);
}

int StreamProfile::id() const { return rs2::stream_profile::unique_id(); }
int StreamProfile::fps() const { return rs2::stream_profile::fps(); }
std::string StreamProfile::name() const {
  return rs2::stream_profile::stream_name();
}
rs2_stream StreamProfile::stream_type() const {
  return rs2::stream_profile::stream_type();
}
rs2_format StreamProfile::format() const {
  return rs2::stream_profile::format();
}

int StreamProfile::width() const {
  const auto video = as<rs2::video_stream_profile>();
  return video ? video.width() : 0;
}

int StreamProfile::height() const {
  const auto video = as<rs2::video_stream_profile>();
  return video ? video.height() : 0;
}

bool StreamProfile::is_video() const {
  const auto video = as<rs2::video_stream_profile>();
  if (video) {
    return true;
  }
  return false;
}

std::string StreamProfile::to_string() const {
  std::ostringstream out;
  out << id() << " | " << name() << " | " << format() << " | " << fps()
      << " FPS";
  if (width() > 0) {
    out << " | " << width() << 'x' << height();
  }
  out << '\n';
  return out.str();
}
