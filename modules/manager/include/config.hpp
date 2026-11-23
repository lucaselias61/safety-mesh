#pragma once

#include "kinematics.hpp"
#include "transform.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/// Where a robot's ST_RobotPose is read over ADS.
struct PlcConfig {
  std::string ip;
  std::string net_id;
  std::uint16_t port;
  std::string symbol;
};

/// Model data shared by every robot of that model, from robots/<model>.json.
struct RobotModel {
  std::string name;
  /// std::nullopt while any DH value is null in the file.
  std::optional<std::vector<DhJoint>> dh;
};

struct RobotConfig {
  std::string id;
  RobotModel model;
  PlcConfig plc;
  /// Maps a point in the robot base frame to the world frame.
  Transform base_to_world;
};

struct CameraConfig {
  std::string id;
  CameraExtrinsics extrinsics; // From cameras/<serial>.json.
};

struct ManagerConfig {
  std::string local_net_id; // AMS Net ID of this machine.
  std::vector<CameraConfig> cameras;
  std::vector<RobotConfig> robots;
};

/// @brief Loads manager.json and the files it names, from its directory.
/// @details
/// - cameras: [{id, serial}]. Each loads cameras/<serial>.json.
/// - robots: [{id, model, base_to_world, plc: {ip, net_id, port, symbol}}].
/// Each loads robots/<model>.json.
/// @throws std::runtime_error naming the file, if any file is missing or
/// invalid.
ManagerConfig load_manager_config(const std::filesystem::path &path);
