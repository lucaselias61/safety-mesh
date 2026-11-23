#pragma once

// Internal to the manager's config loaders.

#include "transform.hpp"

#include <nlohmann/json.hpp>

#include <string>

/// @brief Reads a 4x4 rigid transform written as rows.
/// @param name Key name for error messages, e.g. "camera_to_world".
/// @throws std::runtime_error if not 4x4, the last row is not 0 0 0 1 or the
/// rotation is not a proper rotation.
Transform parse_transform(const nlohmann::json &rows, const std::string &name);
