// Checks decode_robot_pose against both PLC layouts of ST_RobotPose.
// Needs no PLC.

#include "kinematics.hpp"

#include <cstdio>
#include <cstring>
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

// Builds the PLC bytes: nCycle, nJointCount, `padding` zero bytes, then 12
// LREALs 1..12.
std::vector<std::uint8_t>
plc_bytes(std::uint32_t cycle, std::uint8_t joint_count, std::size_t padding) {
  std::vector<std::uint8_t> bytes(sizeof(cycle) + sizeof(joint_count) +
                                  padding);
  std::memcpy(bytes.data(), &cycle, sizeof(cycle));
  bytes[sizeof(cycle)] = joint_count;
  for (int i = 1; i <= 12; ++i) {
    const double value = i;
    const auto *raw = reinterpret_cast<const std::uint8_t *>(&value);
    bytes.insert(bytes.end(), raw, raw + sizeof(value));
  }
  return bytes;
}

void check_pose(const RobotPose &pose, std::uint32_t cycle,
                std::uint8_t joint_count) {
  CHECK(pose.cycle == cycle);
  CHECK(pose.joint_count == joint_count);
  CHECK(pose.tcp_position[0] == 1.0 && pose.tcp_position[2] == 3.0);
  CHECK(pose.tcp_orientation[0] == 4.0 && pose.tcp_orientation[2] == 6.0);
  CHECK(pose.joints[0] == 7.0 && pose.joints[5] == 12.0);
}

bool throws_for_size(std::size_t size) {
  std::vector<std::uint8_t> bytes(size);
  try {
    decode_robot_pose(bytes.data(), bytes.size());
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  const auto packed = plc_bytes(42, 6, 0);
  CHECK(packed.size() == 101);
  check_pose(decode_robot_pose(packed.data(), packed.size()), 42, 6);

  // 3 padding bytes after nJointCount put the LREALs on 8 bytes.
  const auto aligned = plc_bytes(0xFFFFFFFFu, 5, 3);
  CHECK(aligned.size() == 104);
  check_pose(decode_robot_pose(aligned.data(), aligned.size()), 0xFFFFFFFFu, 5);

  CHECK(throws_for_size(0));
  CHECK(throws_for_size(96));
  // Layout before nJointCount.
  CHECK(throws_for_size(100));
  CHECK(throws_for_size(108));

  if (failures != 0) {
    std::printf("robot_pose_test: %d check(s) failed\n", failures);
    return 1;
  }
  std::printf("robot_pose_test: all checks passed\n");
  return 0;
}
