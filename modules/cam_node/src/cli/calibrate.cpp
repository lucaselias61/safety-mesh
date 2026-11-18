#include "camera.hpp"
#include "cli.hpp"
#include "pipeline.hpp"
#include "shutdown.hpp"
#include "utils.hpp"
#include "world_pose.hpp"

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  std::string config;
  std::string board;
  int frames = 30;
  int timeout_s = 30;
};

/// @brief Wraps the color frame's pixels. No copy: use before the frame ends.
/// @throws std::runtime_error for formats other than RGB8 and BGR8.
cv::Mat color_image(const rs2::video_frame &frame) {
  const auto format = frame.get_profile().format();
  if (format != RS2_FORMAT_RGB8 && format != RS2_FORMAT_BGR8)
    throw std::runtime_error(std::string("Color format ") +
                             rs2_format_to_string(format) +
                             " is not supported, use RGB8 or BGR8");
  return cv::Mat(frame.get_height(), frame.get_width(), CV_8UC3,
                 const_cast<void *>(frame.get_data()),
                 static_cast<std::size_t>(frame.get_stride_in_bytes()));
}

/// @brief Feeds color frames to the estimator until enough are used, then
/// solves the pose.
/// @throws std::runtime_error if the camera or a color stream is missing, the
/// time runs out, shutdown is requested or the estimate fails.
world_pose::Result calibrate(const Options &options) {
  const auto camera_config = load_camera_config(options.config);
  const auto board = world_pose::load_board(options.board);
  bool has_color = false;
  for (const auto &stream : camera_config.streams)
    has_color = has_color || stream.name == "Color";
  if (!has_color)
    throw std::runtime_error("Calibration needs an enabled color stream");

  const auto camera = find_camera(camera_config.serial);
  if (!camera)
    throw std::runtime_error("No RealSense device connected with serial " +
                             camera_config.serial);
  Pipeline pipeline(*camera, PipelineProfile::from_streams(
                                 camera->get_device(), camera_config.streams));
  pipeline.start();

  // Built from the first color frame, which carries the active intrinsics.
  std::optional<world_pose::Estimator> estimator;
  int skipped = 0;
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::seconds(options.timeout_s);
  try {
    while (!estimator || estimator->frames() < options.frames) {
      if (shutdown_requested)
        throw std::runtime_error("Calibration interrupted");
      if (std::chrono::steady_clock::now() > deadline)
        throw std::runtime_error(fmt::format(
            "Board seen in {} of {} frames within {} s. Check that the "
            "whole board is visible and matches the board file",
            estimator ? estimator->frames() : 0, options.frames,
            options.timeout_s));
      const auto frames = pipeline.get_frames();
      if (!frames)
        continue;
      const auto color = frames.get_color_frame();
      if (!color)
        continue;
      if (!estimator)
        estimator.emplace(board, color.get_profile()
                                     .as<rs2::video_stream_profile>()
                                     .get_intrinsics());
      if (!estimator->add(color_image(color)))
        ++skipped;
      fmt::print("\rFrames used {}/{}, skipped {}", estimator->frames(),
                 options.frames, skipped);
      std::fflush(stdout);
    }
  } catch (...) {
    fmt::print("\n");
    pipeline.stop();
    throw;
  }
  fmt::print("\n");
  pipeline.stop();
  return estimator->solve(camera->get_serial());
}

void print_result(const world_pose::Result &result) {
  const auto &m = result.camera_to_world;
  const std::time_t time =
      std::chrono::system_clock::to_time_t(result.timestamp);
  char when[32];
  std::strftime(when, sizeof when, "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time));

  fmt::print("Camera {} pose in the board frame\n", result.serial);
  fmt::print("  estimated at   {}\n", when);
  fmt::print("  frames used    {}\n", result.frames);
  fmt::print("  corners used   {}\n", result.corners);
  fmt::print("  reprojection   {:.3f} px\n", result.reprojection_error);
  fmt::print("  position (m)   x {:.4f}  y {:.4f}  z {:.4f}\n", m(0, 3),
             m(1, 3), m(2, 3));
  fmt::print("  camera_to_world (meters)\n");
  for (int row = 0; row < 4; ++row)
    fmt::print("    [{:9.5f} {:9.5f} {:9.5f} {:9.5f}]\n", m(row, 0), m(row, 1),
               m(row, 2), m(row, 3));
}

} // namespace

void calibrate_cmd(CLI::App &app) {
  // The callback owns these objects beyond command registration.
  auto options = std::make_shared<Options>();
  auto *command = app.add_subcommand(
      "calibrate", "Find the camera's pose from the ChArUco board in view");
  command
      ->add_option("--config", options->config,
                   "Path to the camera configuration file")
      ->required()
      ->check(CLI::ExistingFile);
  command
      ->add_option("--board", options->board, "Path to the ChArUco board file")
      ->required()
      ->check(CLI::ExistingFile);
  command
      ->add_option("--frames", options->frames,
                   "Frames with the board to average")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);
  command
      ->add_option("--timeout", options->timeout_s,
                   "Seconds to wait for enough frames")
      ->capture_default_str()
      ->check(CLI::PositiveNumber);

  command->callback([options] { print_result(calibrate(*options)); });
}
