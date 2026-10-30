#include "camera.hpp"
#include "cli.hpp"
#include "engines/engine.hpp"
#include "engines/opencv_dnn_engine.hpp"
#ifdef CAM_NODE_WITH_TENSORRT
#include "engines/tensor_rt_engine.hpp"
#endif
#include "pipeline.hpp"
#include "stream.hpp"
#include "utils.hpp"
#include "worker.hpp"

#include <CLI/CLI.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void run(const std::shared_ptr<std::string> &config) {

  const auto camera_config = load_camera_config(*config);
  const auto engine_config = load_engine_config(*config);
  if (!camera_config.enabled) {
    return;
  }
  if (camera_config.streams.empty()) {
    throw std::runtime_error("No streams enabled in camera configuration");
  }
  for (const auto &camera : discover_cameras()) {
    if (camera.get_serial() == camera_config.serial) {
      PipelineProfile profile;
      for (const auto &stream : camera_config.streams) {
        profile.add(StreamProfile(camera.get_device(), stream));
      };
      std::unique_ptr<Engine> engine;
      if (engine_config.backend == "opencv_dnn") {
        engine = std::make_unique<OpenCVDnnEngine>(engine_config);
      } else if (engine_config.backend == "tensor_rt") {
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
      // Pass the unique_ptr to the worker using std::move. Now engine ==
      // nullptr.
      Worker worker(camera, profile, std::move(engine));
      worker.run();
      return;
    }
  }
  throw std::runtime_error("No RealSense device connected with serial " +
                           camera_config.serial);
}

} // namespace

void run_cmd(CLI::App &app) {
  // The callback owns these objects beyond command registration.
  auto config = std::make_shared<std::string>();
  auto *run_command = app.add_subcommand("run", "Run a connected camera");
  run_command
      ->add_option("--config", *config, "Path to the camera configuration file")
      ->required()
      ->check(CLI::ExistingFile);

  run_command->callback([config] { run(config); });
}
