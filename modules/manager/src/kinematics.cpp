#include "kinematics.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {

constexpr double pi = 3.14159265358979323846;

double radians(double degrees) { return degrees * pi / 180.0; }

Transform link(const DhJoint &joint, double angle_deg) {
  const double theta = radians(angle_deg + joint.theta_offset_deg);
  const double alpha = radians(joint.alpha_deg);
  const double ct = std::cos(theta);
  const double st = std::sin(theta);
  const double ca = std::cos(alpha);
  const double sa = std::sin(alpha);
  return {{{ct, -st * ca, st * sa, joint.a_m * ct},
           {st, ct * ca, -ct * sa, joint.a_m * st},
           {0, sa, ca, joint.d_m},
           {0, 0, 0, 1}}};
}

} // namespace

std::vector<Point3> joint_positions(const std::vector<DhJoint> &dh,
                                    const std::vector<double> &joints_deg,
                                    const Transform &base_to_world) {
  if (joints_deg.size() < dh.size())
    throw std::invalid_argument("Need " + std::to_string(dh.size()) +
                                " joint angles, got " +
                                std::to_string(joints_deg.size()));
  std::vector<Point3> positions;
  positions.reserve(dh.size() + 1);
  Transform frame = base_to_world;
  positions.push_back(apply(frame, {0, 0, 0}));
  for (std::size_t i = 0; i < dh.size(); ++i) {
    frame = compose(frame, link(dh[i], joints_deg[i]));
    positions.push_back(apply(frame, {0, 0, 0}));
  }
  return positions;
}

namespace {

// 12 LREALs on the datagram: 3 x TCP position + 3 x TCP orientation +6 x
// joints.
constexpr std::size_t kDoublesSize = 12 * sizeof(double);
constexpr std::size_t kCycleSize = sizeof(std::uint32_t);
constexpr std::size_t kJointCountSize = sizeof(std::uint8_t);
constexpr std::size_t kPackedSize = kCycleSize + kJointCountSize + kDoublesSize;
constexpr std::size_t kAlignedSize = 8 + kDoublesSize;

// ADS sends the PLC's memory as is. Both the PLC and this host are
// little-endian (x86 / ARM).
template <std::size_t N>
void copy_doubles(const std::uint8_t *&src, std::array<double, N> &dst) {
  std::memcpy(dst.data(), src, N * sizeof(double));
  src += N * sizeof(double);
}

} // namespace

RobotPose decode_robot_pose(const void *data, std::size_t size) {
  if (size != kPackedSize && size != kAlignedSize)
    throw std::invalid_argument("Unexpected ST_RobotPose size " +
                                std::to_string(size) + " bytes; expected " +
                                std::to_string(kPackedSize) + " (packed) or " +
                                std::to_string(kAlignedSize) + " (aligned)");

  const auto *bytes = static_cast<const std::uint8_t *>(data);
  RobotPose pose;
  std::memcpy(&pose.cycle, bytes, sizeof(pose.cycle));
  std::memcpy(&pose.joint_count, bytes + kCycleSize, sizeof(pose.joint_count));

  const std::uint8_t *src = bytes + (size - kDoublesSize);
  copy_doubles(src, pose.tcp_position);
  copy_doubles(src, pose.tcp_orientation);
  copy_doubles(src, pose.joints);
  return pose;
}

RobotPosePoller::RobotPosePoller(comm::AdsClient &client, std::string symbol,
                                 std::chrono::milliseconds stale_after)
    : client_(client), symbol_(std::move(symbol)), stale_after_(stale_after) {}

RobotPose RobotPosePoller::poll() {
  // Cached by the client; re-queried after a PLC restart or online change.
  buffer_.resize(client_.symbol_size(symbol_));
  client_.read(symbol_, buffer_.data(), buffer_.size());
  const RobotPose pose = decode_robot_pose(buffer_.data(), buffer_.size());

  if (!last_cycle_ || pose.cycle != *last_cycle_) {
    last_cycle_ = pose.cycle;
    last_change_ = std::chrono::steady_clock::now();
  }
  return pose;
}

bool RobotPosePoller::is_stale(
    std::chrono::steady_clock::time_point now) const {
  return !last_cycle_ || now - last_change_ > stale_after_;
}
