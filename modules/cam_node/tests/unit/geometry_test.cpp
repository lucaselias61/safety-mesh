// Checks geometry's depth lookups on depth frames made by a librealsense
// software device. Needs no camera.

#include "engines/engine.hpp"
#include "geometry.hpp"

#include <librealsense2/hpp/rs_internal.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
      ++failures;                                                              \
    }                                                                          \
  } while (false)

// Same lens for color and depth.
const rs2_intrinsics lens{
    1280, 720, 640, 360, 900, 900, RS2_DISTORTION_NONE, {0, 0, 0, 0, 0}};

// No rotation, translation x meters.
rs2_extrinsics shift(float x) {
  return {{1, 0, 0, 0, 1, 0, 0, 0, 1}, {x, 0, 0}};
}

// Color 5 cm along x from depth, like a stereo baseline.
geometry::Calibration stereo() {
  return {{lens, lens}, {shift(0.05f), shift(-0.05f)}};
}

// Depth already aligned to color. A color pixel reads the same depth pixel.
geometry::Calibration aligned() { return {{lens, lens}, {shift(0), shift(0)}}; }

// Wall 1 m away. Rows from 500 to 520 have no depth.
constexpr std::uint16_t wall_mm = 1000;
constexpr int no_depth_first_row = 500;
constexpr int no_depth_last_row = 520;

std::size_t index(int x, int y) {
  return static_cast<std::size_t>(y) * static_cast<std::size_t>(lens.width) +
         static_cast<std::size_t>(x);
}

std::vector<std::uint16_t> wall() {
  std::vector<std::uint16_t> pixels(index(0, lens.height), wall_mm);
  for (int y = no_depth_first_row; y <= no_depth_last_row; ++y)
    for (int x = 0; x < lens.width; ++x)
      pixels[index(x, y)] = 0;
  return pixels;
}

// Makes Z16 depth frames in millimeters with a software device.
class DepthSource {
public:
  DepthSource()
      : sensor_(device_.add_sensor("Depth")),
        profile_(sensor_.add_video_stream({RS2_STREAM_DEPTH, 0, 0, lens.width,
                                           lens.height, 30, 2, RS2_FORMAT_Z16,
                                           lens})) {
    sensor_.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
    sensor_.open(profile_);
    sensor_.start(queue_);
  }
  ~DepthSource() {
    sensor_.stop();
    sensor_.close();
  }
  DepthSource(const DepthSource &) = delete;
  DepthSource &operator=(const DepthSource &) = delete;

  /// The frame borrows pixels, which must outlive it.
  rs2::depth_frame frame(std::vector<std::uint16_t> &pixels) {
    sensor_.on_video_frame({pixels.data(), [](void *) {}, lens.width * 2, 2,
                            0.0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK,
                            ++frame_number_, profile_, 0.001f});
    rs2::frame frame;
    if (!queue_.try_wait_for_frame(&frame, 5000))
      throw std::runtime_error("No depth frame from the software device");
    return rs2::depth_frame(frame);
  }

private:
  rs2::software_device device_;
  rs2::software_sensor sensor_;
  // Not const: its conversion to rs2_stream_profile * is non-const.
  rs2::stream_profile profile_;
  rs2::frame_queue queue_;
  int frame_number_ = 0;
};

void test_detection_in_3d() {
  DepthSource source;
  auto pixels = wall();
  const auto depth = source.frame(pixels);

  const Detection2D detection{
      {{400, 300, 0.9f}, {500, 300, 0.3f}, {400, 510, 0.9f}},
      {cv::Rect(350, 250, 200, 300), 0.8f}};
  const auto timestamp = std::chrono::system_clock::now();
  const auto result =
      geometry::detection_in_3d(detection, depth, stereo(), timestamp);

  CHECK(result.confidence == 0.8f);
  CHECK(result.timestamp == timestamp);
  CHECK(result.keypoints.size() == 3);
  for (std::size_t i = 0; i < result.keypoints.size(); ++i)
    CHECK(result.keypoints[i].confidence == detection.keypoints[i].confidence);

  // On the wall: the point the color pixel sees at 1 m, in the color frame.
  // Rounding to the nearest depth pixel costs up to half a pixel, ~0.6 mm.
  const auto &seen = result.keypoints[0].point;
  CHECK(seen.has_value());
  if (seen)
    CHECK(cv::norm(*seen - geometry::deproject(lens, {400, 300}, 1.0f)) <
          0.002);
  // Below keypoint_confidence_threshold.
  CHECK(!result.keypoints[1].point);
  // In the rows without depth.
  CHECK(!result.keypoints[2].point);
}

void test_median_depth() {
  // 5x5 window around (400, 300): 5 spikes at 3 m, 6 holes, 14 pixels on the
  // wall. The center is a spike, so a single pixel read would give 3 m.
  auto pixels = wall();
  const cv::Point spikes[] = {
      {400, 300}, {398, 298}, {402, 298}, {398, 302}, {402, 302}};
  for (const auto &spike : spikes)
    pixels[index(spike.x, spike.y)] = 3000;
  for (int x = 399; x <= 401; ++x) {
    pixels[index(x, 298)] = 0;
    pixels[index(x, 302)] = 0;
  }

  DepthSource source;
  const auto depth = source.frame(pixels);
  const auto point =
      geometry::color_pixel_to_point(depth, {400, 300}, aligned());
  CHECK(point.has_value());
  if (point)
    CHECK(cv::norm(*point - geometry::deproject(lens, {400, 300}, 1.0f)) <
          0.002);
}

} // namespace

int main() {
  test_detection_in_3d();
  test_median_depth();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("geometry tests passed\n");
  return 0;
}
