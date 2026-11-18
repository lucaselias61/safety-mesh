// Checks world_pose against a board rendered at a known camera pose.
// Needs no camera and no printed board.

#include "world_pose.hpp"

#include <opencv2/aruco/charuco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
      ++failures;                                                              \
    }                                                                          \
  } while (false)

template <typename Error, typename Call> bool throws(Call call) {
  try {
    call();
  } catch (const Error &) {
    return true;
  }
  return false;
}

// Placeholder values in it change when the real board is printed, so the
// tests only check that it loads.
const std::string config_board_path =
    std::string(CAM_NODE_CONFIG_DIR) + "/charuco.json";

// Test board, independent of config/charuco.json.
const char *const test_board_json =
    R"({"rows": 7, "columns": 5, "square_size_m": 0.04, "marker_size_m": 0.03, "dictionary": "DICT_5X5_100"})";
// Same dictionary as test_board_json, for rendering.
constexpr auto test_dictionary = cv::aruco::DICT_5X5_100;

std::string temp_path(const std::string &name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

void write_board(const std::string &path, const std::string &json) {
  std::ofstream(path) << json;
}

world_pose::Board test_board() {
  const std::string path = temp_path("world_pose_test_board.json");
  write_board(path, test_board_json);
  return world_pose::load_board(path);
}

rs2_intrinsics intrinsics(rs2_distortion model = RS2_DISTORTION_NONE) {
  return {1280, 720, 640, 360, 900, 900, model, {0, 0, 0, 0, 0}};
}

// Board to camera pose used to render the frames. Board about 0.6 m away,
// tilted, near the image center.
const cv::Vec3d true_rotation(0.25, -0.2, 0.1);
const cv::Vec3d true_translation(-0.1, -0.14, 0.6);

// Renders the board as the camera sees it from the true pose.
cv::Mat render(const world_pose::Board &board, const rs2_intrinsics &color) {
  const int pixels_per_square = 100;
  const auto dictionary = cv::aruco::getPredefinedDictionary(test_dictionary);
  const auto charuco = cv::aruco::CharucoBoard::create(
      board.columns, board.rows, board.square_size, board.marker_size,
      dictionary);
  cv::Mat flat;
  charuco->draw(cv::Size(board.columns * pixels_per_square,
                         board.rows * pixels_per_square),
                flat);

  // Homography from flat board pixels to camera pixels: K [r1 r2 t] S.
  cv::Matx33d r;
  cv::Rodrigues(true_rotation, r);
  const double meters_per_pixel = board.square_size / pixels_per_square;
  const cv::Matx33d camera(color.fx, 0, color.ppx, 0, color.fy, color.ppy, 0, 0,
                           1);
  const cv::Matx33d plane(r(0, 0), r(0, 1), true_translation[0], r(1, 0),
                          r(1, 1), true_translation[1], r(2, 0), r(2, 1),
                          true_translation[2]);
  const cv::Matx33d scale(meters_per_pixel, 0, 0, 0, meters_per_pixel, 0, 0, 0,
                          1);
  cv::Mat image;
  cv::warpPerspective(flat, image, camera * plane * scale,
                      cv::Size(color.width, color.height), cv::INTER_LINEAR,
                      cv::BORDER_CONSTANT, cv::Scalar(255));
  return image;
}

void test_load_board() {
  CHECK(!throws<std::runtime_error>(
      [] { world_pose::load_board(config_board_path); }));

  const auto board = test_board();
  CHECK(board.rows == 7 && board.columns == 5);
  CHECK(std::abs(board.square_size - 0.04f) < 1e-6f);
  CHECK(std::abs(board.marker_size - 0.03f) < 1e-6f);
  CHECK(board.dictionary == "DICT_5X5_100");

  CHECK(throws<std::runtime_error>(
      [] { world_pose::load_board("/nonexistent/charuco.json"); }));

  const std::string path = temp_path("world_pose_test_invalid.json");
  const char *invalid[] = {
      "not json",
      R"({"rows": 7, "columns": 5, "square_size_m": 0.04, "marker_size_m": 0.03})",
      R"({"rows": 0, "columns": 5, "square_size_m": 0.04, "marker_size_m": 0.03, "dictionary": "DICT_5X5_100"})",
      R"({"rows": 7, "columns": 5, "square_size_m": -0.04, "marker_size_m": 0.03, "dictionary": "DICT_5X5_100"})",
      R"({"rows": 7, "columns": 5, "square_size_m": 0.04, "marker_size_m": 0.04, "dictionary": "DICT_5X5_100"})",
      R"({"rows": 7, "columns": 5, "square_size_m": 0.04, "marker_size_m": 0.03, "dictionary": "DICT_9X9_50"})",
  };
  for (const char *json : invalid) {
    write_board(path, json);
    CHECK(throws<std::runtime_error>([&] { world_pose::load_board(path); }));
  }
}

void test_setup_and_frames() {
  const auto board = test_board();
  CHECK(throws<std::invalid_argument>([&] {
    world_pose::Estimator(board, intrinsics(RS2_DISTORTION_KANNALA_BRANDT4));
  }));
  CHECK(!throws<std::invalid_argument>([&] {
    world_pose::Estimator(board, intrinsics(RS2_DISTORTION_BROWN_CONRADY));
  }));

  const auto color = intrinsics();
  world_pose::Estimator estimator(board, color);
  CHECK(throws<std::runtime_error>([&] { estimator.solve("1234"); }));

  const cv::Mat blank(color.height, color.width, CV_8UC1, cv::Scalar(255));
  CHECK(!estimator.add(blank));
  CHECK(estimator.frames() == 0);

  CHECK(throws<std::invalid_argument>(
      [&] { estimator.add(cv::Mat(480, 640, CV_8UC1, cv::Scalar(255))); }));

  CHECK(estimator.add(render(board, color)));
  CHECK(estimator.frames() == 1);

  // More corners required than the board has.
  world_pose::Thresholds strict;
  strict.min_corners_share_to_solve = 2.0;
  world_pose::Estimator starved(board, color, strict);
  CHECK(starved.add(render(board, color)));
  CHECK(throws<std::runtime_error>([&] { starved.solve("1234"); }));
}

void test_known_pose() {
  const auto board = test_board();
  const auto color = intrinsics();
  const cv::Mat image = render(board, color);

  world_pose::Estimator estimator(board, color);
  const int frames = 10;
  for (int i = 0; i < frames; ++i)
    CHECK(estimator.add(image));
  const auto result = estimator.solve("1234");

  // Expected camera to world: R^T and -R^T t of the true pose.
  cv::Matx33d r;
  cv::Rodrigues(true_rotation, r);
  const cv::Matx33d expected_rotation = r.t();
  const cv::Vec3d expected_position = -(expected_rotation * true_translation);

  const cv::Matx44d &m = result.camera_to_world;
  const cv::Matx33d rotation(m(0, 0), m(0, 1), m(0, 2), m(1, 0), m(1, 1),
                             m(1, 2), m(2, 0), m(2, 1), m(2, 2));
  const cv::Vec3d position(m(0, 3), m(1, 3), m(2, 3));

  CHECK(cv::norm(position - expected_position) < 0.005);
  const double cosine =
      (cv::trace(rotation * expected_rotation.t()) - 1.0) / 2.0;
  const double degrees = std::acos(std::min(1.0, cosine)) * 180.0 / CV_PI;
  CHECK(degrees < 0.5);
  CHECK(m(3, 0) == 0 && m(3, 1) == 0 && m(3, 2) == 0 && m(3, 3) == 1);

  // The camera origin lands on the camera position in the board frame.
  const cv::Vec4d origin = m * cv::Vec4d(0, 0, 0, 1);
  CHECK(cv::norm(cv::Vec3d(origin[0], origin[1], origin[2]) -
                 expected_position) < 0.005);

  CHECK(result.reprojection_error < 1.0);
  CHECK(result.serial == "1234");
  CHECK(result.frames == frames);
  CHECK(result.corners == (board.rows - 1) * (board.columns - 1));
  CHECK(result.timestamp <= std::chrono::system_clock::now());

  std::printf("position error %.2f mm, rotation error %.3f deg, "
              "reprojection %.3f px\n",
              cv::norm(position - expected_position) * 1000.0, degrees,
              result.reprojection_error);
}

} // namespace

int main() {
  test_load_board();
  test_setup_and_frames();
  test_known_pose();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("world_pose tests passed\n");
  return 0;
}
