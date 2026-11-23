#include "config.hpp"
#include "json_transform.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace {

nlohmann::json read_json(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("cannot open " + path.string());
  return nlohmann::json::parse(file);
}

/// @return std::nullopt if any DH value is null.
std::optional<std::vector<DhJoint>> parse_dh(const nlohmann::json &joints) {
  std::vector<DhJoint> dh;
  for (const auto &joint : joints) {
    for (const char *key : {"a_m", "alpha_deg", "d_m", "theta_offset_deg"})
      if (joint.at(key).is_null())
        return std::nullopt;
    dh.push_back(
        {joint.at("name").get<std::string>(), joint.at("a_m").get<double>(),
         joint.at("alpha_deg").get<double>(), joint.at("d_m").get<double>(),
         joint.at("theta_offset_deg").get<double>()});
  }
  return dh;
}

RobotModel load_robot_model(const std::filesystem::path &path) {
  try {
    const auto json = read_json(path);
    return {json.at("model").get<std::string>(), parse_dh(json.at("joints"))};
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid robot model file " + path.string() +
                             ": " + error.what());
  }
}

} // namespace

ManagerConfig load_manager_config(const std::filesystem::path &path) {
  const auto directory = path.parent_path();
  nlohmann::json json;
  try {
    json = read_json(path);
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid manager config " + path.string() + ": " +
                             error.what());
  }

  ManagerConfig config;
  try {
    config.local_net_id = json.at("local_net_id").get<std::string>();
    for (const auto &camera : json.at("cameras")) {
      const auto serial = camera.at("serial").get<std::string>();
      config.cameras.push_back(
          {camera.at("id").get<std::string>(),
           load_camera_extrinsics(directory / "cameras" / (serial + ".json"),
                                  serial)});
    }
    for (const auto &robot : json.at("robots")) {
      const auto &plc = robot.at("plc");
      const auto model = robot.at("model").get<std::string>();
      config.robots.push_back(
          {robot.at("id").get<std::string>(),
           load_robot_model(directory / "robots" / (model + ".json")),
           {plc.at("ip").get<std::string>(),
            plc.at("net_id").get<std::string>(),
            plc.at("port").get<std::uint16_t>(),
            plc.at("symbol").get<std::string>()},
           parse_transform(robot.at("base_to_world"), "base_to_world")});
    }
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid manager config " + path.string() + ": " +
                             error.what());
  }
  return config;
}
