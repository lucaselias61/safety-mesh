#pragma once

#include "engines/engine.hpp"
#include "pipeline.hpp"

#include <librealsense2/rs.hpp>
#include <memory>
#include <opencv2/core.hpp>
#include <vector>

/**
 * \brief Builds a BGR preview image from a color frame and a depth heatmap.
 * \param color Color frame; must be RGB8 or BGR8.
 * \param depth Depth frame; when empty only the color image is returned.
 * \param detections Bounding boxes drawn over the color image.
 * \throws std::invalid_argument if the color frame is missing or its format is
 * unsupported.
 */
cv::Mat create_preview(const rs2::video_frame &color,
                       const rs2::video_frame &depth,
                       const std::vector<Detection> &detections = {});

// Displays one frame pair; call repeatedly on the GUI thread.
void view(const std::shared_ptr<Frames> &frames,
          const std::vector<Detection> &detections = {});
