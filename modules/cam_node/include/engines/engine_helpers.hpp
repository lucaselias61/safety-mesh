#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

// Precision the engine computes in. In-/output stays FP32.
enum class Precision { fp32, fp16, int8 };

inline Precision parse_precision(std::string_view name) {
  if (name == "fp32")
    return Precision::fp32;
  if (name == "fp16")
    return Precision::fp16;
  if (name == "int8")
    return Precision::int8;
  throw std::invalid_argument("precision must be fp32, fp16 or int8, not '" +
                              std::string(name) + "'");
}

inline const char *to_string(Precision precision) {
  switch (precision) {
  case Precision::fp32:
    return "fp32";
  case Precision::fp16:
    return "fp16";
  case Precision::int8:
    return "int8";
  }
  return "unknown";
}
