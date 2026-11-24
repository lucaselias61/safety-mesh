#include "camera.hpp"
#include "cli.hpp"
#include "engines/engine.hpp"
#include "engines/opencv_dnn_engine.hpp"
#ifdef CAM_NODE_WITH_TENSORRT
#include "engines/tensorrt/tensorrt_engine.hpp"
#endif
#include "logger.hpp"
#include "pipeline.hpp"
#include "stream.hpp"
#include "utils.hpp"
#include "worker.hpp"

#include <CLI/CLI.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

// Backend when ENGINE_BACKEND is unset: the one this build has.
#ifdef CAM_NODE_WITH_TENSORRT
constexpr const char *default_backend = "tensorrt";
#else
constexpr const char *default_backend = "opencv_dnn";
#endif

/// The camera with this serial, else the only connected camera.
Camera select_camera(const std::string &serial) {
  if (!serial.empty())
    return require_camera(serial);
  auto cameras = discover_cameras().cameras;
  if (cameras.size() != 1)
    throw std::runtime_error("Found " + std::to_string(cameras.size()) +
                             " cameras. Connect one camera or set "
                             "CAMERA_SERIAL.");
  return std::move(cameras.front());
}

void run() {
  const auto camera_config = camera_config_from_env();
  if (!camera_config.enabled) {
    std::cout << "Camera disabled with CAMERA_ENABLED, exiting\n";
    return;
  }
  if (camera_config.streams.empty()) {
    throw std::runtime_error(
        "No streams enabled. Set COLOR_ENABLED or DEPTH_ENABLED.");
  }
  const auto engine_config = engine_config_from_env(default_backend);
  const auto camera = select_camera(camera_config.serial);
  std::cout << "Starting camera " << camera.get_serial() << '\n';
  const auto profile =
      PipelineProfile::from_streams(camera.get_device(), camera_config.streams);
  std::unique_ptr<Engine> engine;
  if (engine_config.backend == "opencv_dnn") {
    engine = std::make_unique<OpenCVDnnEngine>(engine_config);
  } else if (engine_config.backend == "tensorrt") {
#ifdef CAM_NODE_WITH_TENSORRT
    engine = std::make_unique<TensorRTEngine>(engine_config);
#else
    throw std::runtime_error(
        "TensorRT backend requires a build with -DTENSORRT=ON");
#endif
  } else {
    throw std::runtime_error("Unsupported engine backend: " +
                             engine_config.backend);
  }
  // Pass the unique_ptr to the worker using std::move. Now engine == nullptr.
  Worker worker(camera, profile, std::move(engine),
                Logger::from_env(camera.get_serial()));
  worker.run();
}

} // namespace

void run_cmd(CLI::App &app) {
  // Settings come from env vars, see .env.example.
  app.add_subcommand("run", "Run a connected camera, set up by env vars")
      ->callback(run);
}
