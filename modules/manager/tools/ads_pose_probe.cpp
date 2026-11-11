// Prints ST_RobotPose from the robot PLC over ADS until Ctrl+C.
//
//   ./bin/ads_pose_probe --symbol GVL.stRobotPose
//
// Needs a static AMS route on the PLC for this machine (local Net ID + IP).

#include "robot_pose.hpp"
#include "shutdown.hpp"

#include <comm/ads.hpp>

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

int main(int argc, char *argv[]) try {
  std::signal(SIGINT, handle_shutdown);
  std::signal(SIGTERM, handle_shutdown);

  comm::AdsConfig config{"192.168.137.49", "130.149.137.49.1.1", 852,
                         "192.168.137.168.1.1"};
  std::string symbol;
  double rate_hz = 10.0;
  int stale_ms = 500;
  int timeout_ms = 100;

  CLI::App app{"Print the robot pose read from the PLC over ADS",
               "ads_pose_probe"};
  app.add_option("--symbol", symbol, "PLC path of ST_RobotPose")->required();
  app.add_option("--plc-ip", config.plc_ip, "PLC IP address")
      ->capture_default_str();
  app.add_option("--plc-net-id", config.plc_net_id, "PLC AMS Net ID")
      ->capture_default_str();
  app.add_option("--port", config.plc_port, "AMS port (852 = PLC runtime 2)")
      ->capture_default_str();
  app.add_option("--local-net-id", config.local_net_id,
                 "AMS Net ID of this machine (must match the PLC route)")
      ->capture_default_str();
  app.add_option("--rate", rate_hz, "Polls per second")
      ->check(CLI::Range(0.1, 1000.0))
      ->capture_default_str();
  app.add_option("--stale-ms", stale_ms, "Warn if nCycle stops for this long")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--timeout-ms", timeout_ms, "ADS request timeout")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  CLI11_PARSE(app, argc, argv);
  config.timeout = std::chrono::milliseconds(timeout_ms);

  const auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double>(1.0 / rate_hz));
  auto next = std::chrono::steady_clock::now();
  std::optional<std::uint32_t> previous_cycle;
  // Recreated after ADS errors, so the probe survives PLC restarts and
  // cable pulls. Poller before client: it holds a reference to it.
  std::optional<comm::AdsClient> client;
  std::optional<RobotPosePoller> poller;

  while (!shutdown_requested) {
    try {
      if (!client) {
        fmt::print("Connecting to {} ({}) port {} as {}, symbol {}\n",
                   config.plc_ip, config.plc_net_id, config.plc_port,
                   config.local_net_id, symbol);
        client.emplace(config);
        poller.emplace(*client, symbol, std::chrono::milliseconds(stale_ms));
      }
      const RobotPose pose = poller->poll();
      const long long step =
          previous_cycle ? static_cast<long long>(pose.cycle - *previous_cycle)
                         : 0;
      previous_cycle = pose.cycle;
      fmt::print("cycle {:>10} (+{:<4}) tcp [{:9.3f}] abc [{:9.3f}] "
                 "joints [{:8.3f}]{}\n",
                 pose.cycle, step, fmt::join(pose.tcp_position, " "),
                 fmt::join(pose.tcp_orientation, " "),
                 fmt::join(pose.joints, " "),
                 poller->is_stale() ? "  STALE" : "");
    } catch (const comm::AdsError &error) {
      fmt::print(stderr, "ADS failed, reconnecting: {}\n", error.what());
      poller.reset();
      client.reset();
      previous_cycle.reset();
    }
    std::fflush(stdout);
    next += period;
    // Don't try to catch up after a slow reconnect.
    next = std::max(next, std::chrono::steady_clock::now());
    std::this_thread::sleep_until(next);
  }
  return 0;
} catch (const std::exception &error) {
  std::cerr << "(ads_pose_probe error): " << error.what() << '\n';
  return 1;
}
