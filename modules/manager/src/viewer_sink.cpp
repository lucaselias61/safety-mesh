#include "viewer_sink.hpp"

#include <array>
#include <chrono>
#include <utility>

namespace viewer {

rerun::Transform3D transform(const Transform &child_to_parent) {
  // Rerun takes the rotation column-major. Transform is rows.
  std::array<float, 9> rotation{};
  for (std::size_t row = 0; row < 3; ++row)
    for (std::size_t column = 0; column < 3; ++column)
      rotation[column * 3 + row] =
          static_cast<float>(child_to_parent[row][column]);
  const std::array<float, 3> translation{
      static_cast<float>(child_to_parent[0][3]),
      static_cast<float>(child_to_parent[1][3]),
      static_cast<float>(child_to_parent[2][3])};
  return rerun::Transform3D::from_translation_mat3x3(
      rerun::components::Translation3D(translation.data()),
      rerun::components::TransformMat3x3(rotation));
}

} // namespace viewer

namespace {

rerun::components::Position3D position(const Point3 &point) {
  return {static_cast<float>(point[0]), static_cast<float>(point[1]),
          static_cast<float>(point[2])};
}

} // namespace

ViewerSink::ViewerSink(
    std::unique_ptr<rerun_logger::ViewerConnection> connection)
    : viewer_(std::move(connection)) {}

void ViewerSink::cameras(const std::vector<CameraConfig> &cameras) {
  for (const auto &camera : cameras)
    viewer_->log_static("cams/" + camera.extrinsics.serial,
                        viewer::transform(camera.extrinsics.camera_to_world));
}

void ViewerSink::robot(const RobotState &state) {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  viewer_->set_time(
      frame_index_++,
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());

  const std::string root = "robots/" + state.id;
  viewer_->log(root + "/tcp", rerun::Points3D({position(state.tcp)})
                                  .with_radii({viewer::tcp_radius_m}));
  if (!state.joints)
    return;
  std::vector<rerun::components::Position3D> joints;
  std::vector<rerun::encodings::Vec3D> chain;
  for (const auto &joint : *state.joints) {
    joints.push_back(position(joint));
    chain.emplace_back(static_cast<float>(joint[0]),
                       static_cast<float>(joint[1]),
                       static_cast<float>(joint[2]));
  }
  viewer_->log(
      root + "/joints",
      rerun::Points3D(std::move(joints)).with_radii({viewer::joint_radius_m}));
  viewer_->log(
      root + "/links",
      rerun::LineStrips3D(rerun::components::LineStrip3D(std::move(chain)))
          .with_radii({viewer::link_radius_m}));
}
