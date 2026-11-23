#include "run.hpp"
#include "config.hpp"
#include "kinematics.hpp"
#include "logger.hpp"
#include "shutdown.hpp"

#include <comm/ads.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

// The KUKA PLC sends the TCP in millimeters and the joints in degrees.
constexpr double plc_length_m = 0.001;

struct Options {
  std::string config = "config/manager.json";
  double rate_hz = 10.0;
  int stale_ms = 500;
  int timeout_ms = 100;
};

/// One robot and its PLC connection. Recreated after ADS errors.
struct RobotLink {
  const RobotConfig *robot;
  // Client before poller: the poller holds a reference to it.
  std::optional<comm::AdsClient> client;
  std::optional<RobotPosePoller> poller;
  bool failing = false; // Warned once until the next good poll.
};

/// @return The robot in the world frame.
/// @throws std::invalid_argument if the PLC sends fewer joints than the DH.
RobotState world_state(const RobotConfig &robot, const RobotPose &pose,
                       bool stale) {
  const Point3 tcp_base{pose.tcp_position[0] * plc_length_m,
                        pose.tcp_position[1] * plc_length_m,
                        pose.tcp_position[2] * plc_length_m};
  RobotState state{robot.id, pose.cycle, std::nullopt,
                   apply(robot.base_to_world, tcp_base), stale};
  if (robot.model.dh) {
    const std::size_t count =
        std::min<std::size_t>(pose.joint_count, pose.joints.size());
    const std::vector<double> joints(pose.joints.begin(),
                                     pose.joints.begin() + count);
    state.joints =
        joint_positions(*robot.model.dh, joints, robot.base_to_world);
  }
  return state;
}

void poll(RobotLink &link, const ManagerConfig &config, const Options &options,
          Logger &logger) {
  const auto &robot = *link.robot;
  try {
    if (!link.client) {
      link.client.emplace(comm::AdsConfig{
          robot.plc.ip, robot.plc.net_id, robot.plc.port, config.local_net_id,
          std::chrono::milliseconds(options.timeout_ms)});
      link.poller.emplace(*link.client, robot.plc.symbol,
                          std::chrono::milliseconds(options.stale_ms));
    }
    const RobotPose pose = link.poller->poll();
    logger.robot(world_state(robot, pose, link.poller->is_stale()));
    if (link.failing)
      logger.info("Robot " + robot.id + ": reading again");
    link.failing = false;
  } catch (const std::exception &error) {
    if (!link.failing)
      logger.warn("Robot " + robot.id + ": " + error.what() +
                  "; retrying every poll");
    link.failing = true;
    // ADS errors may leave the connection broken. Others are data errors.
    if (dynamic_cast<const comm::AdsError *>(&error)) {
      link.poller.reset();
      link.client.reset();
    }
  }
}

void run(const Options &options) {
  Logger logger = Logger::from_env();
  const ManagerConfig config = load_manager_config(options.config);
  logger.cameras(config.cameras);

  // Built once and never resized: each poller refers to its link's client.
  std::vector<RobotLink> links;
  links.reserve(config.robots.size());
  for (const auto &robot : config.robots) {
    if (!robot.model.dh)
      logger.warn(fmt::format("Robot {}: DH values of {} are not set; logging "
                              "the TCP only",
                              robot.id, robot.model.name));
    links.push_back({&robot, std::nullopt, std::nullopt, false});
  }

  const auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double>(1.0 / options.rate_hz));
  auto next = std::chrono::steady_clock::now();
  while (!shutdown_requested) {
    for (auto &link : links)
      poll(link, config, options, logger);
    next += period;
    // Don't try to catch up after a slow reconnect.
    next = std::max(next, std::chrono::steady_clock::now());
    std::this_thread::sleep_until(next);
  }
}

} // namespace

void run_cmd(CLI::App &app) {
  // The callback owns these options beyond command registration.
  auto options = std::make_shared<Options>();
  auto *command = app.add_subcommand(
      "run", "Log camera poses and robots in the world frame until stopped");
  command->add_option("--config", options->config, "Path to manager.json")
      ->capture_default_str()
      ->check(CLI::ExistingFile);
  command->add_option("--rate", options->rate_hz, "Robot polls per second")
      ->capture_default_str()
      ->check(CLI::Range(0.1, 1000.0));
  command
      ->add_option("--stale-ms", options->stale_ms,
                   "Mark a robot stale if its PLC cycle stops this long")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
  command
      ->add_option("--timeout-ms", options->timeout_ms, "ADS request timeout")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
  command->callback([options] { run(*options); });
}
