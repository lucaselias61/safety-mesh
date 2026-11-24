#pragma once

#include "engines/tensorrt/tensorrt_builder.hpp"
#include "stream.hpp"

#include <string>
#include <vector>

// Filled from env vars by camera_config_from_env.
struct CameraConfig {
  std::string serial;
  bool enabled;
  std::vector<StreamConfig> streams;
};

// Always sorts by ID; "fps", "width", "height" or "resolution" sort within each
// ID. The key "all" sorts by ID only. Keys are case-insensitive. Throws
// std::invalid_argument if the key is unknown.
std::vector<StreamProfile>
sort_stream_profiles_by_key(const std::vector<StreamProfile> &profiles,
                            std::string key);

// Filters stream profiles by the given type.
// The filter can be "all", "color", "depth", or "infrared".
// Throws std::invalid_argument if the filter is unknown.
std::vector<StreamProfile>
filter_stream_profiles(const std::vector<StreamProfile> &profiles,
                       std::string filter);

struct EngineConfig {
  std::string backend;
  std::string model_path;
  double confidence_threshold;
  double nms_threshold;
  int input_size;
  Precision precision = Precision::fp32; // Optional. Default fp32.
};

// Camera and engine settings from env vars. Compose passes them from .env.
// - Every function throws std::runtime_error naming the variable that is
//   missing or invalid. An empty value counts as missing.
// - See .env.example for the variables and their values.

/// Color and depth streams, always both. WIDTH, HEIGHT and FPS apply to both.
std::vector<StreamConfig> streams_from_env();

/// Camera settings. serial is CAMERA_SERIAL, empty to use the only camera.
/// streams is empty when CAMERA_ENABLED is false.
CameraConfig camera_config_from_env();

/// Engine settings. ENGINE_BACKEND is optional, default_backend if unset.
EngineConfig engine_config_from_env(const std::string &default_backend);
