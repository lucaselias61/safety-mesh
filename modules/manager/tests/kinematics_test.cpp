// Checks joint_positions on arms whose joint positions are known by hand.

#include "kinematics.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {

int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
      ++failures;                                                              \
    }                                                                          \
  } while (false)

bool near(const Point3 &a, const Point3 &b) {
  return std::abs(a[0] - b[0]) < 1e-9 && std::abs(a[1] - b[1]) < 1e-9 &&
         std::abs(a[2] - b[2]) < 1e-9;
}

// Two 1 m links turning about z, in the x-y plane.
const std::vector<DhJoint> planar{{"A1", 1.0, 0, 0, 0}, {"A2", 1.0, 0, 0, 0}};

void test_planar() {
  auto points = joint_positions(planar, {0, 0}, identity_transform);
  CHECK(points.size() == 3);
  CHECK(near(points[0], {0, 0, 0}));
  CHECK(near(points[1], {1, 0, 0}));
  CHECK(near(points[2], {2, 0, 0}));

  // Second joint at 90 deg bends the arm up along y.
  points = joint_positions(planar, {0, 90}, identity_transform);
  CHECK(near(points[2], {1, 1, 0}));

  // theta_offset adds to the joint angle.
  const std::vector<DhJoint> offset{{"A1", 1.0, 0, 0, 90}};
  CHECK(near(joint_positions(offset, {0}, identity_transform)[1], {0, 1, 0}));
}

void test_d_and_alpha() {
  // d lifts the first frame 0.5 m along z. alpha = 90 tips it so its y points
  // up. With theta 90, the second 1 m link runs along that y, straight up.
  const std::vector<DhJoint> arm{{"A1", 0, 90, 0.5, 0}, {"A2", 1.0, 0, 0, 90}};
  const auto points = joint_positions(arm, {0, 0}, identity_transform);
  CHECK(near(points[1], {0, 0, 0.5}));
  CHECK(near(points[2], {0, 0, 1.5}));
}

void test_base_to_world() {
  // Base 2 m along world x, turned 90 deg about z.
  const Transform base{
      {{0, -1, 0, 2}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
  const auto points = joint_positions(planar, {0, 0}, base);
  CHECK(near(points[0], {2, 0, 0}));
  CHECK(near(points[2], {2, 2, 0}));
}

void test_too_few_angles() {
  try {
    joint_positions(planar, {0}, identity_transform);
    CHECK(false);
  } catch (const std::invalid_argument &) {
  }
  // Extra angles are ignored.
  CHECK(joint_positions(planar, {0, 0, 45}, identity_transform).size() == 3);
}

} // namespace

int main() {
  test_planar();
  test_d_and_alpha();
  test_base_to_world();
  test_too_few_angles();
  if (failures > 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("kinematics tests passed\n");
  return 0;
}
