#include "camera.hpp"
#include "cli.hpp"
#include "pipeline.hpp"
#include "preview.hpp"
#include "shutdown.hpp"
#include "stream.hpp"

#include <CLI/CLI.hpp>
#include <memory>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <string>

namespace {

void view(const std::shared_ptr<std::string> &serial) {
  for (const auto &camera : discover_cameras()) {
    if (camera.get_serial() != *serial) {
      continue;
    }
    PipelineProfile profile;
    profile.add(StreamProfile(camera.get_device(),
                              StreamConfig{"Color", 640, 480, 15, "RGB8"}));
    profile.add(StreamProfile(camera.get_device(),
                              StreamConfig{"Depth", 640, 480, 15, "Z16"}));
    Pipeline pipeline(camera, profile);
    pipeline.start();

    const std::string window = "Camera " + *serial;
    try {
      cv::namedWindow(window, cv::WINDOW_AUTOSIZE);
      while (!shutdown_requested) {
        const auto frames = pipeline.get_frames();
        const auto color = frames.get_color_frame();
        const auto depth = frames.get_depth_frame();
        if (color && depth) {
          cv::imshow(window, create_preview(color, depth));
        }
        const int key = cv::waitKey(1);
        if (key == 27 || key == 'q' ||
            cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
          break;
        }
      }
      cv::destroyWindow(window);
    } catch (...) {
      cv::destroyAllWindows();
      pipeline.stop();
      throw;
    }
    pipeline.stop();
    return;
  }
  throw std::runtime_error("No RealSense device connected with serial " +
                           *serial);
}

} // namespace

void view_cmd(CLI::App &app) {
  auto serial = std::make_shared<std::string>();
  auto *command = app.add_subcommand(
      "view", "Preview color and depth heatmap side by side");
  command->add_option("--serial", *serial, "Camera serial number")
      ->required()
      ->check([](const std::string &value) {
        return value.empty() ? "Serial number cannot be empty" : "";
      });

  command->callback([serial] { view(serial); });
}
