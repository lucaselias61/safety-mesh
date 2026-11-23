#include "transform.hpp"
#include "json_transform.hpp"

#include <nlohmann/json.hpp>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

/// @throws std::runtime_error if text is not YYYY-MM-DDTHH:MM:SSZ.
std::chrono::system_clock::time_point parse_utc(const std::string &text) {
  std::tm utc{};
  std::istringstream in(text);
  in >> std::get_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  if (in.fail())
    throw std::runtime_error("estimated_at is not UTC ISO 8601: " + text);
  return std::chrono::system_clock::from_time_t(timegm(&utc));
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Transform compose(const Transform &a, const Transform &b) {
  Transform result{};
  for (std::size_t row = 0; row < 4; ++row)
    for (std::size_t column = 0; column < 4; ++column)
      for (std::size_t k = 0; k < 4; ++k)
        result[row][column] += a[row][k] * b[k][column];
  return result;
}

Point3 apply(const Transform &t, const Point3 &p) {
  Point3 result{};
  for (std::size_t row = 0; row < 3; ++row)
    result[row] =
        t[row][0] * p[0] + t[row][1] * p[1] + t[row][2] * p[2] + t[row][3];
  return result;
}

CameraExtrinsics load_camera_extrinsics(const std::filesystem::path &path,
                                        const std::string &expected_serial) {
  try {
    std::ifstream file(path);
    if (!file)
      throw std::runtime_error("cannot open file");
    const auto json = nlohmann::json::parse(file);
    CameraExtrinsics extrinsics{
        json.at("serial").get<std::string>(),
        parse_transform(json.at("camera_to_world"), "camera_to_world"),
        json.at("reprojection_error_px").get<double>(),
        parse_utc(json.at("estimated_at").get<std::string>())};
    if (extrinsics.serial != expected_serial)
      throw std::runtime_error("serial " + extrinsics.serial +
                               " does not match " + expected_serial);
    return extrinsics;
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid camera pose file " + path.string() +
                             ": " + error.what());
  }
}
