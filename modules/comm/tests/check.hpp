#pragma once

// Minimal test helpers: CHECK records a failure and continues, so one run
// reports every failed check. Return finish() from main.

#include <cstdio>

namespace test {

inline int failures = 0;

inline int finish(const char *name) {
  if (failures != 0) {
    std::printf("%s: %d check(s) failed\n", name, failures);
    return 1;
  }
  std::printf("%s: all checks passed\n", name);
  return 0;
}

} // namespace test

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
      ++test::failures;                                                        \
    }                                                                          \
  } while (false)
