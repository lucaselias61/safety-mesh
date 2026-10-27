#include "utils.hpp"
#include "stream.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

int positive_int(const nlohmann::json &object, const char *key) {
  const auto &value = object.at(key);
  if (!value.is_number_integer() || value < 1 ||
      value > std::numeric_limits<int>::max()) {
    throw std::runtime_error(std::string(key) + " must be a positive integer");
  }
  return value.get<int>();
}

std::string to_lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return text;
}

nlohmann::json load_json(const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Cannot open file");
  }
  return nlohmann::json::parse(file);
}

bool file_exists(const std::string &path) {
  std::ifstream file(path);
  return static_cast<bool>(file);
}

} // namespace

CameraConfig load_camera_config(const std::string &path) {
  try {
    const auto json = load_json(path);
    const auto &camera_json = json.at("camera");
    CameraConfig config{camera_json.at("serial").get<std::string>(),
                        camera_json.at("enabled").get<bool>(),
                        {}};
    if (config.serial.empty()) {
      throw std::runtime_error("serial must not be empty");
    }
    const auto &streams = camera_json.at("streams");
    if (!streams.is_object()) {
      throw std::runtime_error("streams must be an object");
    }
    for (const auto &entry : streams.items()) {
      try {
        const auto &value = entry.value();
        if (!value.at("enabled").get<bool>()) {
          continue;
        }
        std::string name;
        for (int type = 1; type < RS2_STREAM_COUNT; ++type) {
          const std::string sdk_name =
              rs2_stream_to_string(static_cast<rs2_stream>(type));
          if (to_lowercase(entry.key()) == to_lowercase(sdk_name)) {
            name = sdk_name;
            break;
          }
        }
        if (name.empty()) {
          throw std::runtime_error("Unknown stream name");
        }
        StreamConfig stream{
            name, value.contains("width") ? positive_int(value, "width") : 0,
            value.contains("height") ? positive_int(value, "height") : 0,
            positive_int(value, "fps"), value.at("format").get<std::string>()};
        if (stream.format.empty()) {
          throw std::runtime_error("format must not be empty");
        }
        config.streams.push_back(std::move(stream));
      } catch (const std::exception &error) {
        throw std::runtime_error("stream '" + entry.key() +
                                 "': " + error.what());
      }
    }
    return config;
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid camera config '" + path +
                             "': " + error.what());
  }
}

std::vector<StreamProfile>
sort_stream_profiles_by_key(const std::vector<StreamProfile> &profiles,
                            std::string key) {
  key = to_lowercase(key);
  if (key != "all" && key != "fps" && key != "width" && key != "height" && key != "resolution") {
    throw std::invalid_argument("Unknown sort key: " + key);
  }

  auto result = profiles;
  std::stable_sort(result.begin(), result.end(),
                   [&key](const StreamProfile &a, const StreamProfile &b) {
                     if (a.id() != b.id())
                       return a.id() < b.id();
                     if (key == "fps")
                       return a.fps() < b.fps();
                     if (key == "width")
                       return a.width() < b.width();
                     if (key == "height")
                       return a.height() < b.height();
                     if (key == "resolution")
                       return (a.width() * a.height()) < (b.width() * b.height());
                     return false;
                   });
  return result;
}

std::vector<StreamProfile>
filter_stream_profiles(const std::vector<StreamProfile> &profiles,
                       std::string filter) {
  filter = to_lowercase(filter);

  auto result = profiles;
  if (filter == "color" || filter == "depth" || filter == "infrared") {
    const auto type = filter == "color"   ? RS2_STREAM_COLOR
                      : filter == "depth" ? RS2_STREAM_DEPTH
                                          : RS2_STREAM_INFRARED;
    result.erase(std::remove_if(result.begin(), result.end(),
                                [type](const StreamProfile &stream) {
                                  return stream.stream_type() != type;
                                }),
                 result.end());
  } else if (filter != "all") {
    throw std::invalid_argument("Unknown filter: " + filter);
  }
  return result;
}

EngineConfig load_engine_config(const std::string &path) {
  try {
    const auto json = load_json(path);
    const auto &engine_json = json.at("engine");
    EngineConfig config{
        engine_json.at("backend").get<std::string>(),
        engine_json.at("model_path").get<std::string>(),
        engine_json.at("confidence_threshold").get<double>(),
        engine_json.at("nms_threshold").get<double>(),
        positive_int(engine_json, "input_size"),
    };
    if (config.model_path.empty()) {
      throw std::runtime_error("model_path must not be empty");
    }
    if (!file_exists(config.model_path)) {
      throw std::runtime_error("model_path does not exist: " +
                               config.model_path);
    }
    if (config.backend.empty()) {
      throw std::runtime_error("backend must not be empty");
    }
    if (config.confidence_threshold <= 0.0 ||
        config.confidence_threshold > 1.0) {
      throw std::runtime_error("confidence_threshold must be between 0 and 1");
    }
    if (config.nms_threshold <= 0.0 || config.nms_threshold > 1.0) {
      throw std::runtime_error("nms_threshold must be between 0 and 1");
    }
    return config;
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid engine config '" + path +
                             "': " + error.what());
  }
}
