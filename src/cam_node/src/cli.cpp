#include "cli.hpp"
#include "utils.hpp"
#include "camera.hpp"
#include "worker.hpp"
#include "shutdown.hpp"
#include "engines/opencv_dnn_engine.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <CLI/CLI.hpp>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>


void list_cmd(CLI::App& app) {
    app.add_subcommand("list", "List connected RealSense cameras")->callback([]() {
        const auto cameras = discover_cameras();
        if (cameras.empty()) {
            std::cout << "No RealSense device connected\n";
        }
        for (const auto& camera : cameras) {
            std::cout << camera.get_serial() << '\t' << camera.get_name() << '\n';
        }
    });
}


void view_cmd(CLI::App& app) {
    auto serial = std::make_shared<std::string>();
    auto* command = app.add_subcommand("view", "Preview color and depth heatmap side by side");
    command->add_option("--serial", *serial, "Camera serial number")
        ->required()->check([](const std::string& value) {
            return value.empty() ? "Serial number cannot be empty" : "";
        });

    command->callback([serial] {
        for (const auto& camera : discover_cameras()) {
            if (camera.get_serial() != *serial) {continue;}
            PipelineProfile profile;
            profile.add(StreamProfile(camera.get_device(), StreamConfig{"Color", 640, 480, 15, "RGB8"}));
            profile.add(StreamProfile(camera.get_device(), StreamConfig{"Depth", 640, 480, 15, "Z16"}));
            Pipeline pipeline(camera, profile);
            rs2::colorizer colorizer;
            pipeline.start();

            const std::string window = "Camera " + *serial;
            cv::Mat preview(480, 1280, CV_8UC3);
            cv::Mat color_panel = preview(cv::Rect(0, 0, 640, 480));
            cv::Mat depth_panel = preview(cv::Rect(640, 0, 640, 480));
            try {
                cv::namedWindow(window, cv::WINDOW_AUTOSIZE);
                while (!shutdown_requested) {
                    const auto frames = pipeline.get_frames();
                    const auto color = frames.get_color_frame();
                    const auto depth = frames.get_depth_frame();
                    if (color && depth) {
                        cv::Mat rgb(color.get_height(), color.get_width(), CV_8UC3, const_cast<void*>(color.get_data()), color.get_stride_in_bytes());
                        cv::cvtColor(rgb, color_panel, cv::COLOR_RGB2BGR);

                        const rs2::video_frame heatmap = colorizer.colorize(depth);
                        cv::Mat depth_rgb(heatmap.get_height(), heatmap.get_width(), CV_8UC3,
                            const_cast<void*>(heatmap.get_data()), heatmap.get_stride_in_bytes());
                        cv::cvtColor(depth_rgb, depth_panel, cv::COLOR_RGB2BGR);
                        cv::imshow(window, preview);
                    }
                    const int key = cv::waitKey(1);
                    if (key == 27 || key == 'q' || cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
                        break;
                    }
                }
                cv::destroyWindow(window);
            } catch (...) {
                cv::destroyAllWindows();
                throw;
            }
            pipeline.stop();
            return;
        }
        throw std::runtime_error("No RealSense device connected with serial " + *serial);
    });
}


void run_cmd(CLI::App& app) {
    // The callback owns these objects beyond command registration.
    auto config = std::make_shared<std::string>();
    auto* run_command = app.add_subcommand("run", "Run a connected camera");
    run_command->add_option("--config", *config, "Path to the camera configuration file")
    ->required()
    ->check(CLI::ExistingFile);

    run_command->callback([config] {
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
    });
}


void streams_cmd(CLI::App& app) {
    auto serial = std::make_shared<std::string>();
    auto* streams_command = app.add_subcommand("streams", "List supported stream profiles for a connected camera");
    streams_command->add_option("--serial", *serial, "Specify the serial number of the camera")
    ->required()
    ->check([](const std::string& value) {return value.empty() ? "Serial number cannot be empty" : "";});
    auto key = std::make_shared<std::string>("all");
    streams_command->add_option("--sort", *key, "Sort within each stream ID (all, fps, width, height)");
    auto filter = std::make_shared<std::string>("all");
    streams_command->add_option("--filter", *filter, "Filter stream profiles by type (all, color, depth, infrared)"); 

    streams_command->callback([serial, key, filter] {
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
    });
}
