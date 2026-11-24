// Live test of the camera pipeline cam_node runs, for every connected camera.
// Uses cam_node's own config loading, Pipeline and frame timing.
// - Streams from the env vars cam_node run reads (WIDTH, HEIGHT, FPS,
//   COLOR_*, DEPTH_*), the same for every camera. See .env.example.
// Cameras are tested one after another. Steps print PASS or FAIL with the
// serial. A failed step that later steps depend on ends that camera's test.
// Exit code 1 if any step failed or no camera is connected.

#include "camera.hpp"
#include "frames.hpp"
#include "pipeline.hpp"
#include "stream.hpp"
#include "utils.hpp"

#include <librealsense2/rs.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

// Frames are collected for this long once the pipeline runs.
constexpr auto measure_window = 5s;
// Measured rate must reach this share of the configured fps.
constexpr double min_fps_share = 0.9;
// Share of frames that may be missing, from gaps in frame numbers.
constexpr double max_drop_share = 0.05;
// Frame age limit, in frame periods of the stream.
constexpr double max_age_periods = 3.0;
// Global time mapping jitter can put the capture slightly after now.
constexpr double min_age_ms = -5.0;
// Depth pixels with a distance, as a share of the image.
constexpr double min_depth_valid_share = 0.05;
// Spread between the darkest and brightest color byte.
constexpr int min_color_spread = 10;

int passed = 0;
int failed = 0;

/// Ends the current step with message if condition is false.
void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

/// Runs one step and prints PASS or FAIL. @return True if it passed.
bool step(const std::string &name, const std::function<void()> &body) {
  try {
    body();
    std::printf("PASS %s\n", name.c_str());
    ++passed;
    return true;
  } catch (const std::exception &error) {
    std::printf("FAIL %s: %s\n", name.c_str(), error.what());
    ++failed;
    return false;
  }
}

/// Prints the summary. @return The process exit code.
int finish() {
  std::printf("\n%d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}

/// Device or sensor info, or "unknown" if not reported.
template <typename Source>
std::string info(const Source &source, rs2_camera_info field) {
  return source.supports(field) ? source.get_info(field) : "unknown";
}

std::string fixed(double value, int decimals = 1) {
  char text[32];
  std::snprintf(text, sizeof text, "%.*f", decimals, value);
  return text;
}

std::string describe(const StreamConfig &stream) {
  std::string text = stream.name + " " + stream.format;
  if (stream.width > 0)
    text += " " + std::to_string(stream.width) + "x" +
            std::to_string(stream.height);
  return text + " @ " + std::to_string(stream.fps) + " fps";
}

/// What one stream delivered during the measuring window.
struct StreamStats {
  rs2::stream_profile profile;
  int frames = 0;
  unsigned long long first_number = 0;
  unsigned long long last_number = 0;
  double last_timestamp = 0;
  bool timestamps_increase = true;
  bool all_global = true;
  std::vector<double> ages_ms;
  rs2::frame last;
};

/// Stats of the delivered stream of the configured type, or nullptr.
const StreamStats *find_stats(const std::map<int, StreamStats> &streams,
                              const StreamConfig &config) {
  for (const auto &[id, stats] : streams)
    if (config.name == rs2_stream_to_string(stats.profile.stream_type()))
      return &stats;
  return nullptr;
}

/// Collects every frame for measure_window.
/// @return Stats per stream profile, and the number of empty framesets.
std::map<int, StreamStats> measure(Pipeline &pipeline, int &timeouts,
                                   double &seconds) {
  std::map<int, StreamStats> streams;
  timeouts = 0;
  const auto start = Clock::now();
  while (Clock::now() - start < measure_window) {
    const auto frames = pipeline.get_frames();
    if (!frames) {
      ++timeouts;
      continue;
    }
    for (const rs2::frame &frame : frames.frameset()) {
      auto &stats = streams[frame.get_profile().unique_id()];
      const auto number = frame.get_frame_number();
      if (stats.frames > 0 && number == stats.last_number)
        continue; // Same frame in a later set.
      if (stats.frames == 0) {
        stats.profile = frame.get_profile();
        stats.first_number = number;
      } else if (frame.get_timestamp() <= stats.last_timestamp) {
        stats.timestamps_increase = false;
      }
      if (frame.get_frame_timestamp_domain() ==
          RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME)
        stats.ages_ms.push_back(
            std::chrono::duration<double, std::milli>(frame_age(frame))
                .count());
      else
        stats.all_global = false;
      stats.last_number = number;
      stats.last_timestamp = frame.get_timestamp();
      stats.last = frame;
      ++stats.frames;
    }
  }
  seconds = std::chrono::duration<double>(Clock::now() - start).count();
  return streams;
}

/// Checks one configured stream against what it delivered.
void check_stream(const StreamConfig &config, const StreamStats *stats,
                  double seconds) {
  require(stats != nullptr, "no frames received");
  const auto &profile = stats->profile;
  require(rs2_format_to_string(profile.format()) == config.format,
          std::string("format is ") + rs2_format_to_string(profile.format()));
  require(profile.fps() == config.fps,
          "profile runs at " + std::to_string(profile.fps()) + " fps");
  if (const auto video = profile.as<rs2::video_stream_profile>())
    require(video.width() == config.width && video.height() == config.height,
            "resolution is " + std::to_string(video.width()) + "x" +
                std::to_string(video.height()));

  const double rate = stats->frames / seconds;
  const double expected =
      static_cast<double>(stats->last_number - stats->first_number + 1);
  const double drop_share = 1.0 - stats->frames / expected;
  auto ages = stats->ages_ms;
  std::sort(ages.begin(), ages.end());
  const double min_age = ages.empty() ? 0 : ages.front();
  const double median_age = ages.empty() ? 0 : ages[ages.size() / 2];
  const double max_age = ages.empty() ? 0 : ages.back();
  std::printf("  %s: %d frames, %s fps, %s%% dropped, age median %s ms, "
              "max %s ms\n",
              describe(config).c_str(), stats->frames, fixed(rate).c_str(),
              fixed(drop_share * 100).c_str(), fixed(median_age).c_str(),
              fixed(max_age).c_str());

  require(rate >= min_fps_share * config.fps,
          "rate " + fixed(rate) + " fps, below " +
              fixed(min_fps_share * config.fps) + " fps");
  require(drop_share <= max_drop_share,
          fixed(drop_share * 100) + "% of frames dropped");
  require(stats->timestamps_increase, "timestamps do not increase");
  require(stats->all_global, "frames not in global time");
  const double age_limit = max_age_periods * 1000.0 / config.fps;
  require(max_age <= age_limit, "frame age up to " + fixed(max_age) +
                                    " ms, limit " + fixed(age_limit) + " ms");
  require(min_age >= min_age_ms,
          "frame age down to " + fixed(min_age) + " ms, clocks disagree");
}

/// The color image is not flat, so the sensor sees something.
void check_color(const rs2::video_frame &frame) {
  const auto *data = static_cast<const std::uint8_t *>(frame.get_data());
  const auto size = static_cast<std::size_t>(frame.get_stride_in_bytes()) *
                    static_cast<std::size_t>(frame.get_height());
  const auto [low, high] = std::minmax_element(data, data + size);
  require(*high - *low >= min_color_spread,
          "image is flat (bytes " + std::to_string(*low) + " to " +
              std::to_string(*high) + "), lens covered or sensor dark");
}

/// Enough depth pixels carry a distance.
void check_depth(const rs2::depth_frame &frame) {
  const auto *data = static_cast<const std::uint16_t *>(frame.get_data());
  const auto pixels = static_cast<std::size_t>(frame.get_width()) *
                      static_cast<std::size_t>(frame.get_height());
  const auto valid = std::count_if(data, data + pixels,
                                   [](std::uint16_t value) { return value; });
  const double share = static_cast<double>(valid) / pixels;
  require(share >= min_depth_valid_share,
          fixed(share * 100) + "% of pixels have depth, need " +
              fixed(min_depth_valid_share * 100) + "%");
}

bool configures(const CameraConfig &config, const char *name) {
  return std::any_of(
      config.streams.begin(), config.streams.end(),
      [name](const StreamConfig &stream) { return stream.name == name; });
}

/// Runs every step for one camera.
void test_camera(const Camera &camera,
                 const std::vector<StreamConfig> &streams) {
  const std::string tag = "[" + camera.get_serial() + "] ";
  const auto &device = camera.get_device();
  std::printf("\n%s%s, firmware %s, USB %s\n", tag.c_str(),
              camera.get_name().c_str(),
              info(device, RS2_CAMERA_INFO_FIRMWARE_VERSION).c_str(),
              info(device, RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR).c_str());

  const CameraConfig config{camera.get_serial(), true, streams};

  if (!step(tag + "camera ready", [&] {
        require(camera.is_ready(), "disconnected or in recovery mode");
      }))
    return;

  step(tag + "USB 3", [&] {
    const auto usb = info(device, RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
    require(!usb.empty() && usb.front() == '3',
            "connected over USB " + usb + ", streams need USB 3");
  });

  PipelineProfile profile;
  std::vector<StreamProfile> wanted;
  if (!step(tag + "configured streams supported", [&] {
        for (const auto &stream : config.streams) {
          try {
            const StreamProfile match(device, stream);
            profile.add(match);
            wanted.push_back(match);
          } catch (const std::exception &) {
            throw std::runtime_error(describe(stream) +
                                     " is not offered by the camera");
          }
        }
      }))
    return;

  if (!step(tag + "global time supported by streaming sensors", [&] {
        for (const auto &stream : wanted) {
          bool found = false;
          for (const auto &sensor : device.query_sensors())
            for (const auto &offered : sensor.get_stream_profiles())
              if (offered.unique_id() == stream.id()) {
                found = true;
                require(sensor.supports(RS2_OPTION_GLOBAL_TIME_ENABLED),
                        info(sensor, RS2_CAMERA_INFO_NAME) +
                            " does not support global time");
              }
          require(found, "no sensor offers " + stream.name());
        }
      }))
    return;

  // start() turns on global time and waits for frames in it, as in cam_node.
  std::optional<Pipeline> pipeline;
  if (!step(tag + "pipeline starts in global time", [&] {
        pipeline.emplace(camera, profile);
        pipeline->start();
      }))
    return;

  if (configures(config, "Color") && configures(config, "Depth")) {
    step(tag + "calibration available", [&] {
      const auto &calibration = pipeline->calibration();
      require(calibration.has_value(), "no depth and color calibration");
      for (const auto &stream : config.streams) {
        if (stream.name != "Color" && stream.name != "Depth")
          continue;
        const auto &intrinsics = stream.name == "Color"
                                     ? calibration->intrinsics.color
                                     : calibration->intrinsics.depth;
        require(intrinsics.width == stream.width &&
                    intrinsics.height == stream.height,
                stream.name + " intrinsics are " +
                    std::to_string(intrinsics.width) + "x" +
                    std::to_string(intrinsics.height));
      }
    });
  } else {
    std::printf("SKIP %scalibration available: needs color and depth\n",
                tag.c_str());
  }

  std::map<int, StreamStats> streams;
  double seconds = 0;
  const bool arriving = step(tag + "frames arrive", [&] {
    int timeouts = 0;
    streams = measure(*pipeline, timeouts, seconds);
    require(timeouts == 0,
            std::to_string(timeouts) + " waits for frames timed out");
    require(!streams.empty(), "no frames in " + fixed(seconds) + " s");
  });

  if (arriving) {
    for (const auto &stream : config.streams)
      step(tag + "stream healthy: " + stream.name,
           [&] { check_stream(stream, find_stats(streams, stream), seconds); });

    for (const auto &stream : config.streams) {
      const auto *stats = find_stats(streams, stream);
      if (!stats)
        continue;
      if (stream.name == "Color")
        step(tag + "color image has content",
             [&] { check_color(stats->last.as<rs2::video_frame>()); });
      else if (stream.name == "Depth")
        step(tag + "depth image has content",
             [&] { check_depth(stats->last.as<rs2::depth_frame>()); });
    }
  }

  streams.clear(); // Release the held frames before stopping.
  step(tag + "pipeline stops", [&] { pipeline->stop(); });
}

} // namespace

int main() {
  std::vector<StreamConfig> streams;
  if (!step("streams from env", [&] { streams = streams_from_env(); }))
    return finish();

  std::vector<Camera> cameras;
  if (!step("cameras connected", [&] {
        cameras = discover_cameras().cameras;
        require(!cameras.empty(), "no RealSense camera connected");
      }))
    return finish();
  std::printf("  %zu camera(s)\n", cameras.size());

  for (const auto &camera : cameras)
    test_camera(camera, streams);
  return finish();
}
