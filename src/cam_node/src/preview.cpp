#include "preview.hpp"
#include "shutdown.hpp"

#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <string>

cv::Mat create_preview(const rs2::video_frame &color,
                       const rs2::video_frame &depth,
                       const std::vector<Detection> &detections) {
  if (!color)
    throw std::invalid_argument("Preview requires a color frame");
  cv::Mat color_mat(color.get_height(), color.get_width(), CV_8UC3,
                    const_cast<void *>(color.get_data()),
                    color.get_stride_in_bytes());
  cv::Mat color_bgr;
  const auto format = color.get_profile().format();
  if (format == RS2_FORMAT_RGB8) {
    cv::cvtColor(color_mat, color_bgr, cv::COLOR_RGB2BGR);
  } else if (format == RS2_FORMAT_BGR8) {
    color_bgr = color_mat.clone();
  } else {
    throw std::invalid_argument("Preview requires RGB8 or BGR8 color");
  }

  for (const auto &detection : detections) {
    const auto &b = detection.bbox;
    if (b.empty())
      continue;
    cv::rectangle(color_bgr, b, cv::Scalar(0, 255, 0), 2);
  }
  if (!depth)
    return color_bgr;

  static thread_local rs2::colorizer colorizer;
  const rs2::video_frame heatmap = colorizer.colorize(depth);
  cv::Mat depth_mat(heatmap.get_height(), heatmap.get_width(), CV_8UC3,
                    const_cast<void *>(heatmap.get_data()),
                    heatmap.get_stride_in_bytes());
  cv::Mat depth_bgr;
  cv::cvtColor(depth_mat, depth_bgr, cv::COLOR_RGB2BGR);
  cv::resize(depth_bgr, depth_bgr, color_bgr.size());

  cv::Mat preview;
  cv::hconcat(color_bgr, depth_bgr, preview);
  return preview;
}

void view(const std::shared_ptr<Frames> &frames,
          const std::vector<Detection> &detections) {
  if (!frames || !*frames)
    return;
  const auto color = frames->get_color_frame();
  if (!color)
    return;
  const std::string window = "Detections";
  cv::imshow(window,
             create_preview(color, frames->get_depth_frame(), detections));
  const int key = cv::waitKey(1);
  if (key == 27 || key == 'q' ||
      cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
    shutdown_requested = 1;
    cv::destroyWindow(window);
  }
}
