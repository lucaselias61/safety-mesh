#include "utils.hpp"
#include "stream.hpp"

#include <librealsense2/rs.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

std::string to_lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return text;
}

/// @return The value of the env var, or "" if unset.
std::string env(const std::string &name) {
  const char *value = std::getenv(name.c_str());
  return value ? value : "";
}

std::string required(const std::string &name) {
  auto value = env(name);
  if (value.empty())
    throw std::runtime_error("Set " + name + ", see .env.example");
  return value;
}

[[noreturn]] void invalid(const std::string &name, const std::string &value,
                          const std::string &expected) {
  throw std::runtime_error(name + " must be " + expected + ", got '" + value +
                           "'");
}

bool required_bool(const std::string &name) {
  auto value = required(name);
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (value == "true" || value == "1" || value == "yes" || value == "on")
    return true;
  if (value == "false" || value == "0" || value == "no" || value == "off")
    return false;
  invalid(name, value, "true or false");
}

int positive_int(const std::string &name) {
  const auto value = required(name);
  std::size_t used = 0;
  long number = 0;
  try {
    number = std::stol(value, &used);
  } catch (const std::exception &) {
    used = 0;
  }
  if (used != value.size() || number < 1 ||
      number > std::numeric_limits<int>::max())
    invalid(name, value, "a positive integer");
  return static_cast<int>(number);
}

/// A number greater than 0 and at most 1.
double share(const std::string &name) {
  const auto value = required(name);
  std::size_t used = 0;
  double number = 0.0;
  try {
    number = std::stod(value, &used);
  } catch (const std::exception &) {
    used = 0;
  }
  if (used != value.size() || number <= 0.0 || number > 1.0)
    invalid(name, value, "greater than 0 and at most 1");
  return number;
}

} // namespace

std::vector<StreamProfile>
sort_stream_profiles_by_key(const std::vector<StreamProfile> &profiles,
                            std::string key) {
  key = to_lowercase(key);
  if (key != "all" && key != "fps" && key != "width" && key != "height" &&
      key != "resolution") {
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
                       return (a.width() * a.height()) <
                              (b.width() * b.height());
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

std::vector<StreamConfig> streams_from_env() {
  const int width = positive_int("WIDTH");
  const int height = positive_int("HEIGHT");
  const int fps = positive_int("FPS");
  std::vector<StreamConfig> streams;
  for (const auto &[prefix, type] :
       {std::pair{"COLOR", RS2_STREAM_COLOR}, {"DEPTH", RS2_STREAM_DEPTH}}) {
    const std::string name = prefix;
    if (!required_bool(name + "_ENABLED"))
      continue;
    // SDK stream names: "Color", "Depth".
    streams.push_back({rs2_stream_to_string(type), width, height, fps,
                       required(name + "_FORMAT")});
  }
  return streams;
}

CameraConfig camera_config_from_env() {
  CameraConfig config{
      env("CAMERA_SERIAL"), required_bool("CAMERA_ENABLED"), {}};
  if (config.enabled)
    config.streams = streams_from_env();
  return config;
}

EngineConfig engine_config_from_env(const std::string &default_backend) {
  const auto backend = env("ENGINE_BACKEND");
  const auto precision = env("PRECISION");
  EngineConfig config{
      backend.empty() ? default_backend : backend,
      required("MODEL_PATH"),
      share("CONFIDENCE_THRESHOLD"),
      share("NMS_THRESHOLD"),
      positive_int("INPUT_SIZE"),
      precision.empty() ? Precision::fp32 : parse_precision(precision),
  };
  if (!std::ifstream(config.model_path))
    throw std::runtime_error("MODEL_PATH does not exist: " + config.model_path);
  return config;
}
