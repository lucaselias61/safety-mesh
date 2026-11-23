// Checks load_camera_extrinsics on pose files in cam_node calibrate's format.
// Needs no camera.

#include "camera_extrinsics.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
      ++failures;                                                              \
    }                                                                          \
  } while (false)

// Rotation of 90 deg about z, translation 0.1 0.2 0.3.
const char *const valid_rows = R"([[0, -1, 0, 0.1], [1, 0, 0, 0.2],
                                   [0, 0, 1, 0.3], [0, 0, 0, 1]])";

std::string
pose_json(const std::string &rows, const std::string &serial = "1234",
          const std::string &estimated_at = "2025-11-18T03:12:45Z") {
  return R"({"serial": ")" + serial + R"(", "estimated_at": ")" + estimated_at +
         R"(", "frames": 30, "corners": 24, "reprojection_error_px": 0.25,
            "camera_to_world": )" +
         rows + "}";
}

std::string write(const std::string &json) {
  auto path = (std::filesystem::temp_directory_path() /
               "manager_camera_extrinsics_test.json")
                  .string();
  std::ofstream(path) << json;
  return path;
}

bool rejects(const std::string &json, const std::string &serial = "1234") {
  try {
    load_camera_extrinsics(write(json), serial);
  } catch (const std::runtime_error &) {
    return true;
  }
  return false;
}

void test_valid() {
  const auto extrinsics =
      load_camera_extrinsics(write(pose_json(valid_rows)), "1234");
  CHECK(extrinsics.serial == "1234");
  CHECK(extrinsics.reprojection_error_px == 0.25);
  CHECK(extrinsics.estimated_at == std::chrono::system_clock::time_point(
                                       std::chrono::seconds(1763435565)));
  // Rows as written, not transposed.
  CHECK(extrinsics.camera_to_world[0][1] == -1.0);
  CHECK(extrinsics.camera_to_world[1][0] == 1.0);
  CHECK(extrinsics.camera_to_world[2][3] == 0.3);
}

void test_invalid() {
  CHECK(rejects(pose_json(valid_rows), "5678"));
  CHECK(rejects("not json"));
  CHECK(rejects(R"({"serial": "1234"})"));
  CHECK(rejects(pose_json(valid_rows, "1234", "yesterday")));
  // 3 rows.
  CHECK(rejects(pose_json("[[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0]]")));
  // Last row not 0 0 0 1.
  CHECK(rejects(
      pose_json("[[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 1, 1]]")));
  // Scaled, not a rotation.
  CHECK(rejects(
      pose_json("[[2, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]")));
  // Mirrored: orthonormal but det = -1.
  CHECK(rejects(
      pose_json("[[-1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]")));
  CHECK(rejects(pose_json(valid_rows), "1234") == false);
  try {
    load_camera_extrinsics("/nonexistent/pose.json", "1234");
    CHECK(false);
  } catch (const std::runtime_error &) {
  }
}

} // namespace

int main() {
  test_valid();
  test_invalid();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("camera_extrinsics tests passed\n");
  return 0;
}
