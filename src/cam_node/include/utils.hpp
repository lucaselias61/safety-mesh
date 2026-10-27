#pragma once

#include "stream.hpp"

#include <string>
#include <vector>

struct CameraConfig {
  std::string serial;
  bool enabled;
  std::vector<StreamConfig> streams;
};

// Throws std::runtime_error if the file cannot be read or is invalid.
CameraConfig load_camera_config(const std::string &path);

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
};

// Throws std::runtime_error if the file cannot be read or is invalid.
EngineConfig load_engine_config(const std::string &path);