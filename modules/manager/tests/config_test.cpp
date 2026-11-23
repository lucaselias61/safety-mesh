// Checks load_manager_config on a config directory written to a temp folder.

#include "config.hpp"

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

namespace fs = std::filesystem;

const char *const identity =
    "[[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]";

std::string manager_json(const std::string &base_to_world = identity) {
  return std::string(R"({
    "local_net_id": "1.2.3.4.1.1",
    "cameras": [{"id": "cam_1", "serial": "1234"}],
    "robots": [{
      "id": "robot_1", "model": "arm",
      "base_to_world": )") +
         base_to_world + R"(,
      "plc": {"ip": "10.0.0.2", "net_id": "10.0.0.2.1.1", "port": 852,
              "symbol": "GVL.stRobotPose"}
    }]
  })";
}

const char *const camera_json = R"({
  "serial": "1234", "estimated_at": "2025-11-18T03:12:45Z", "frames": 30,
  "corners": 24, "reprojection_error_px": 0.2,
  "camera_to_world": [[1, 0, 0, 0.5], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
})";

std::string model_json(const std::string &a_m) {
  return R"({"model": "arm", "joints": [
    {"name": "A1", "a_m": )" +
         a_m +
         R"(, "alpha_deg": 0, "d_m": 0.4, "theta_offset_deg": 0,
     "min_deg": null, "max_deg": null}]})";
}

void write(const fs::path &path, const std::string &text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << text;
}

/// Fresh config directory. Returns the manager.json path.
fs::path config_dir(const std::string &manager, const std::string &model,
                    bool with_camera = true) {
  const auto dir = fs::temp_directory_path() / "manager_config_test";
  fs::remove_all(dir);
  write(dir / "manager.json", manager);
  write(dir / "robots" / "arm.json", model);
  if (with_camera)
    write(dir / "cameras" / "1234.json", camera_json);
  return dir / "manager.json";
}

bool rejects(const fs::path &path) {
  try {
    load_manager_config(path);
  } catch (const std::runtime_error &) {
    return true;
  }
  return false;
}

void test_valid() {
  const auto config =
      load_manager_config(config_dir(manager_json(), model_json("0.025")));
  CHECK(config.local_net_id == "1.2.3.4.1.1");
  CHECK(config.cameras.size() == 1);
  CHECK(config.cameras[0].id == "cam_1");
  CHECK(config.cameras[0].extrinsics.camera_to_world[0][3] == 0.5);
  CHECK(config.robots.size() == 1);
  const auto &robot = config.robots[0];
  CHECK(robot.id == "robot_1");
  CHECK(robot.plc.port == 852);
  CHECK(robot.plc.symbol == "GVL.stRobotPose");
  CHECK(robot.model.name == "arm");
  CHECK(robot.model.dh.has_value());
  if (robot.model.dh) {
    CHECK(robot.model.dh->size() == 1);
    CHECK((*robot.model.dh)[0].a_m == 0.025);
    CHECK((*robot.model.dh)[0].d_m == 0.4);
  }
}

void test_null_dh() {
  // Null DH values load, without kinematics.
  const auto config =
      load_manager_config(config_dir(manager_json(), model_json("null")));
  CHECK(!config.robots[0].model.dh.has_value());
}

void test_invalid() {
  // Listed camera without a pose file.
  CHECK(rejects(config_dir(manager_json(), model_json("0"), false)));
  // Not a rotation.
  CHECK(rejects(config_dir(
      manager_json("[[2, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]"),
      model_json("0"))));
  // Missing key.
  CHECK(
      rejects(config_dir(R"({"cameras": [], "robots": []})", model_json("0"))));
  CHECK(rejects(fs::temp_directory_path() / "missing" / "manager.json"));
}

} // namespace

int main() {
  test_valid();
  test_null_dh();
  test_invalid();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("config tests passed\n");
  return 0;
}
