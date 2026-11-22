// Checks geometry::detection_in_3d on depth frames made by a librealsense
// software device. Needs no camera.

#include "engines/engine.hpp"
#include "geometry.hpp"

#include <librealsense2/hpp/rs_internal.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
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

// Color 5 cm along x from depth, no rotation, like a stereo baseline.
geometry::Calibration calibration() {
  const rs2_extrinsics color_to_depth{{1, 0, 0, 0, 1, 0, 0, 0, 1},
                                      {0.05f, 0, 0}};
  const rs2_extrinsics depth_to_color{{1, 0, 0, 0, 1, 0, 0, 0, 1},
                                      {-0.05f, 0, 0}};
  return {{lens, lens}, {color_to_depth, depth_to_color}};
}

// Wall 1 m away. Rows from 500 to 520 have no depth.
constexpr std::uint16_t wall_mm = 1000;
constexpr int no_depth_first_row = 500;
constexpr int no_depth_last_row = 520;

std::vector<std::uint16_t> wall() {
  std::vector<std::uint16_t> pixels(static_cast<std::size_t>(lens.width) *
                                        static_cast<std::size_t>(lens.height),
                                    wall_mm);
  for (int y = no_depth_first_row; y <= no_depth_last_row; ++y)
    for (int x = 0; x < lens.width; ++x)
      pixels[static_cast<std::size_t>(y * lens.width + x)] = 0;
  return pixels;
}

void test_detection_in_3d() {
  rs2::software_device device;
  auto sensor = device.add_sensor("Depth");
  // Not const: its conversion to rs2_stream_profile * is non-const.
  auto profile =
      sensor.add_video_stream({RS2_STREAM_DEPTH, 0, 0, lens.width, lens.height,
                               30, 2, RS2_FORMAT_Z16, lens});
  sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
  rs2::frame_queue queue;
  sensor.open(profile);
  sensor.start(queue);

  // The frame borrows pixels, which outlive it.
  auto pixels = wall();
  sensor.on_video_frame({pixels.data(), [](void *) {}, lens.width * 2, 2, 0.0,
                         RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 1, profile,
                         0.001f});
  rs2::frame frame;
  CHECK(queue.try_wait_for_frame(&frame, 5000));
  const rs2::depth_frame depth(frame);
  CHECK(depth);

  const Detection2D detection{
      {{400, 300, 0.9f}, {500, 300, 0.3f}, {400, 510, 0.9f}},
      {cv::Rect(350, 250, 200, 300), 0.8f}};
  const auto timestamp = std::chrono::system_clock::now();
  const auto result =
      geometry::detection_in_3d(detection, depth, calibration(), timestamp);

  CHECK(result.confidence == 0.8f);
  CHECK(result.timestamp == timestamp);
  CHECK(result.keypoints.size() == 3);
  for (std::size_t i = 0; i < result.keypoints.size(); ++i)
    CHECK(result.keypoints[i].confidence == detection.keypoints[i].confidence);

  // On the wall: the point the color pixel sees at 1 m, in the color frame.
  // Rounding to the nearest depth pixel costs up to half a pixel, ~0.6 mm.
  const auto &seen = result.keypoints[0].point;
  CHECK(seen.has_value());
  if (seen) {
    const cv::Point3f expected = geometry::deproject(lens, {400, 300}, 1.0f);
    CHECK(cv::norm(*seen - expected) < 0.002);
  }
  // Below keypoint_confidence_threshold.
  CHECK(!result.keypoints[1].point);
  // In the rows without depth.
  CHECK(!result.keypoints[2].point);

  sensor.stop();
  sensor.close();
}

} // namespace

int main() {
  test_detection_in_3d();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("geometry tests passed\n");
  return 0;
}
