#include "json_transform.hpp"

#include <cmath>
#include <stdexcept>

namespace {

// Allowed error of R * R^T against identity and of det(R) against 1.
constexpr double rotation_tolerance = 1e-3;

/// @throws std::runtime_error if not a rigid transform.
void validate(const Transform &t, const std::string &name) {
  if (t[3][0] != 0 || t[3][1] != 0 || t[3][2] != 0 || t[3][3] != 1)
    throw std::runtime_error(name + " last row must be 0 0 0 1");
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j) {
      double dot = 0;
      for (std::size_t k = 0; k < 3; ++k)
        dot += t[i][k] * t[j][k];
      if (std::abs(dot - (i == j ? 1.0 : 0.0)) > rotation_tolerance)
        throw std::runtime_error(name + " rotation is not orthonormal");
    }
  const double det = t[0][0] * (t[1][1] * t[2][2] - t[1][2] * t[2][1]) -
                     t[0][1] * (t[1][0] * t[2][2] - t[1][2] * t[2][0]) +
                     t[0][2] * (t[1][0] * t[2][1] - t[1][1] * t[2][0]);
  if (std::abs(det - 1.0) > rotation_tolerance)
    throw std::runtime_error(name + " rotation is a reflection");
}

} // namespace

Transform parse_transform(const nlohmann::json &rows, const std::string &name) {
  if (!rows.is_array() || rows.size() != 4)
    throw std::runtime_error(name + " must have 4 rows");
  Transform transform{};
  for (std::size_t row = 0; row < 4; ++row) {
    const auto &values = rows[row];
    if (!values.is_array() || values.size() != 4)
      throw std::runtime_error(name + " rows must have 4 values");
    for (std::size_t column = 0; column < 4; ++column)
      transform[row][column] = values[column].get<double>();
  }
  validate(transform, name);
  return transform;
}
