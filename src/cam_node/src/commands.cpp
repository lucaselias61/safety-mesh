#include "commands.hpp"
#include "camera.hpp"
#include "pipeline.hpp"
#include "shutdown.hpp"
#include "utils.hpp"
#include "worker.hpp"
#include "engines/engine.hpp"
#include "engines/opencv_dnn_engine.hpp"

#include <iostream>
#include <stdexcept>
#include <opencv2/opencv.hpp>

namespace {

    cv::Mat create_preview(const rs2::video_frame& color, const rs2::video_frame& depth, const std::vector<Detection>& detections = {}) {
        if (!color) throw std::invalid_argument("Preview requires a color frame");
        cv::Mat color_mat(color.get_height(), color.get_width(), CV_8UC3, const_cast<void*>(color.get_data()), color.get_stride_in_bytes());
        cv::Mat color_bgr;
        const auto format = color.get_profile().format();
        if (format == RS2_FORMAT_RGB8) {cv::cvtColor(color_mat, color_bgr, cv::COLOR_RGB2BGR);} 
        else if (format == RS2_FORMAT_BGR8) {color_bgr = color_mat.clone();} 
        else {
            throw std::invalid_argument("Preview requires RGB8 or BGR8 color");
        }

        for (const auto& detection : detections) {
            const auto& b = detection.bbox;
            if (b.size() != 4 || b[2] <= b[0] || b[3] <= b[1]) continue;
            cv::rectangle(color_bgr, cv::Point(b[0], b[1]), cv::Point(b[2], b[3]), cv::Scalar(0, 255, 0), 2);
        }
        if (!depth) return color_bgr;

        static thread_local rs2::colorizer colorizer;
        const rs2::video_frame heatmap = colorizer.colorize(depth);
        cv::Mat depth_mat(heatmap.get_height(), heatmap.get_width(), CV_8UC3, const_cast<void*>(heatmap.get_data()), heatmap.get_stride_in_bytes());
        cv::Mat depth_bgr;
        cv::cvtColor(depth_mat, depth_bgr, cv::COLOR_RGB2BGR);
        cv::resize(depth_bgr, depth_bgr, color_bgr.size());

        cv::Mat preview;
        cv::hconcat(color_bgr, depth_bgr, preview);
        return preview;
    }
}


void list() {
    const auto cameras = discover_cameras();
    if (cameras.empty()) {
        std::cout << "No RealSense device connected\n";
    }
    for (const auto& camera : cameras) {
        std::cout << camera.get_serial() << '\t' << camera.get_name() << '\n';
    }
}

void view(const std::shared_ptr<std::string>& serial) {
    for (const auto& camera : discover_cameras()) {
        if (camera.get_serial() != *serial) {continue;}
        PipelineProfile profile;
        profile.add(StreamProfile(camera.get_device(), StreamConfig{"Color", 640, 480, 15, "RGB8"}));
        profile.add(StreamProfile(camera.get_device(), StreamConfig{"Depth", 640, 480, 15, "Z16"}));
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
                if (key == 27 || key == 'q' || cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
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
    throw std::runtime_error("No RealSense device connected with serial " + *serial);
}


void view(const std::shared_ptr<Frames>& frames, const std::vector<Detection>& detections) {
    if (!frames || !*frames) return;
    const auto color = frames->get_color_frame();
    if (!color) return;
    const std::string window = "Detections";
    cv::imshow(window, create_preview(color, frames->get_depth_frame(), detections));
    const int key = cv::waitKey(1);
    if (key == 27 || key == 'q' || cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
        shutdown_requested = 1;
        cv::destroyWindow(window);
    }
}

void run(const std::shared_ptr<std::string>& config) {

    const auto camera_config = load_camera_config(*config);
    const auto engine_config = load_engine_config(*config);
    if (!camera_config.enabled) {
        return;
    }
    if (camera_config.streams.empty()) {
        throw std::runtime_error("No streams enabled in camera configuration");
    }
    for (const auto& camera : discover_cameras()) {
        if (camera.get_serial() == camera_config.serial) {
            PipelineProfile profile;
            for (const auto& stream : camera_config.streams) {
                profile.add(StreamProfile(camera.get_device(), stream));
            };
            std::unique_ptr<Engine> engine;
            if (engine_config.backend == "opencv_dnn") {
                engine = std::make_unique<OpenCVDnnEngine>(engine_config);
            }
            else {
                throw std::runtime_error("Unsupported engine backend: " + engine_config.backend);
            }
            // Pass the unique_ptr to the worker using std::move. Now engine == nullptr.
            Worker worker(camera, profile, std::move(engine));
            worker.run();
            return;
        }
    }
    throw std::runtime_error("No RealSense device connected with serial " + camera_config.serial);
}

void streams(const std::shared_ptr<std::string>& serial, const std::shared_ptr<std::string>& key, const std::shared_ptr<std::string>& filter) {
    for (const auto& camera : discover_cameras()) {
        if (camera.get_serial() == *serial) {
            const auto streams = sort_stream_profiles_by_key(
                filter_stream_profiles(camera.get_supported_stream_profiles(), *filter), *key);

            if (streams.empty()) {
                std::cout << "No supported stream profiles found.\n";
                return;
            }

            for (const auto& stream : streams) {
                std::cout << stream.to_string();
            }
            return;
        }// Exit the callback after processing the matching camera
    }
    throw std::runtime_error("No RealSense device connected with serial " + *serial);

}
