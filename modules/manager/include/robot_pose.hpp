#pragma once

#include <comm/ads.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// Mirrors ST_RobotPose in modules/plc/DUTs/ST_RobotPose.st.
/// Units are whatever the PLC sends.
struct RobotPose {
  std::uint32_t cycle = 0;      // +1 per PLC write
  std::uint8_t joint_count = 0; // Number of valid joints in the joints array
  std::array<double, 3> tcp_position{}; // Cartesian coordinates in robot base
                                        // (fTcpX, fTcpY, fTcpZ)
  std::array<double, 3>
      tcp_orientation{}; // Orientation in robot base (fTcpA, fTcpB, fTcpC)
  std::array<double, 6> joints{}; // aJoints[0..5]
};

/// @brief Decodes the raw bytes sent by the PLC.
/// @details PLC layouts:
/// - packed (100 bytes, pack_mode 1)
/// - 8-byte aligned (104 bytes, 4 bytes of padding after nCycle).
/// @throws std::invalid_argument for any other size.
RobotPose decode_robot_pose(const void *data, std::size_t size);

/// @brief Polls from the robot PLC and tracks updates. Not thread-safe.
class RobotPosePoller {
public:
  /// @param client Connection to the robot PLC; must outlive the poller.
  /// @param symbol PLC path of the pose variable, e.g. "GVL.stRobotPose".
  /// @param stale_after How long cycle may stay unchanged before the counts
  /// stales.
  RobotPosePoller(comm::AdsClient &client, std::string symbol,
                  std::chrono::milliseconds stale_after);

  /// @throws comm::AdsError on communication failures,
  /// std::invalid_argument on unexpected size.
  RobotPose poll();

  /// @brief True if nothing was read yet or cycle has not advanced for
  /// longer than stale_after.
  bool is_stale(std::chrono::steady_clock::time_point now =
                    std::chrono::steady_clock::now()) const;

private:
  comm::AdsClient &client_;
  std::string symbol_;
  std::chrono::milliseconds stale_after_;
  std::vector<std::uint8_t> buffer_;
  std::optional<std::uint32_t> last_cycle_;
  std::chrono::steady_clock::time_point last_change_{};
};
