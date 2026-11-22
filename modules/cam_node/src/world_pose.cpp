#include "world_pose.hpp"

#include <nlohmann/json.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/calib3d.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>

namespace world_pose {
namespace {

int dictionary_id(const std::string &name) {
  static const std::map<std::string, int> ids = {
      {"DICT_4X4_50", cv::aruco::DICT_4X4_50},
      {"DICT_4X4_100", cv::aruco::DICT_4X4_100},
      {"DICT_4X4_250", cv::aruco::DICT_4X4_250},
      {"DICT_4X4_1000", cv::aruco::DICT_4X4_1000},
      {"DICT_5X5_50", cv::aruco::DICT_5X5_50},
      {"DICT_5X5_100", cv::aruco::DICT_5X5_100},
      {"DICT_5X5_250", cv::aruco::DICT_5X5_250},
      {"DICT_5X5_1000", cv::aruco::DICT_5X5_1000},
      {"DICT_6X6_50", cv::aruco::DICT_6X6_50},
      {"DICT_6X6_100", cv::aruco::DICT_6X6_100},
      {"DICT_6X6_250", cv::aruco::DICT_6X6_250},
      {"DICT_6X6_1000", cv::aruco::DICT_6X6_1000},
      {"DICT_7X7_50", cv::aruco::DICT_7X7_50},
      {"DICT_7X7_100", cv::aruco::DICT_7X7_100},
      {"DICT_7X7_250", cv::aruco::DICT_7X7_250},
      {"DICT_7X7_1000", cv::aruco::DICT_7X7_1000},
  };
  const auto id = ids.find(name);
  if (id == ids.end())
    throw std::invalid_argument("unknown dictionary " + name);
  return id->second;
}

void validate(const Board &board) {
  if (board.rows < 2 || board.columns < 2)
    throw std::invalid_argument("rows and columns must be at least 2");
  if (!(board.square_size > 0) || !(board.marker_size > 0))
    throw std::invalid_argument("square and marker size must be positive");
  if (board.marker_size >= board.square_size)
    throw std::invalid_argument("marker must be smaller than the square");
  dictionary_id(board.dictionary);
}

// Coefficients in OpenCV order k1 k2 p1 p2 k3.
cv::Vec<double, 5> distortion(const rs2_intrinsics &intrinsics) {
  switch (intrinsics.model) {
  case RS2_DISTORTION_NONE:
    return {};
  case RS2_DISTORTION_BROWN_CONRADY:
  case RS2_DISTORTION_MODIFIED_BROWN_CONRADY:
  case RS2_DISTORTION_INVERSE_BROWN_CONRADY:
    return {intrinsics.coeffs[0], intrinsics.coeffs[1], intrinsics.coeffs[2],
            intrinsics.coeffs[3], intrinsics.coeffs[4]};
  case RS2_DISTORTION_FTHETA:
    throw std::invalid_argument("Unsupported distortion model F-Theta");
  case RS2_DISTORTION_KANNALA_BRANDT4:
    throw std::invalid_argument("Unsupported distortion model Kannala-Brandt");
  default:
    throw std::invalid_argument("Unsupported distortion model " +
                                std::to_string(intrinsics.model));
  }
}

} // namespace

Board load_board(const std::string &path) {
  try {
    std::ifstream file(path);
    if (!file)
      throw std::runtime_error("cannot open file");
    const auto json = nlohmann::json::parse(file);
    Board board{json.at("rows").get<int>(), json.at("columns").get<int>(),
                json.at("square_size_m").get<float>(),
                json.at("marker_size_m").get<float>(),
                json.at("dictionary").get<std::string>()};
    validate(board);
    return board;
  } catch (const std::exception &error) {
    throw std::runtime_error("Invalid board file " + path + ": " +
                             error.what());
  }
}

Estimator::Estimator(const Board &board, const rs2_intrinsics &color,
                     const Thresholds &thresholds)
    : thresholds_(thresholds), size_(color.width, color.height),
      camera_matrix_(color.fx, 0, color.ppx, 0, color.fy, color.ppy, 0, 0, 1),
      distortion_(distortion(color)) {
  validate(board);
  dictionary_ =
      cv::aruco::getPredefinedDictionary(dictionary_id(board.dictionary));
  board_ = cv::aruco::CharucoBoard::create(board.columns, board.rows,
                                           board.square_size, board.marker_size,
                                           dictionary_);
  sums_.resize(board_->chessboardCorners.size());
  counts_.resize(board_->chessboardCorners.size());
}

bool Estimator::add(const cv::Mat &image) {
  if (image.depth() != CV_8U ||
      (image.channels() != 1 && image.channels() != 3))
    throw std::invalid_argument("Image must be 8-bit gray or color");
  if (image.size() != size_)
    throw std::invalid_argument("Image size does not match the intrinsics");

  std::vector<std::vector<cv::Point2f>> marker_corners;
  std::vector<int> marker_ids;
  cv::aruco::detectMarkers(image, dictionary_, marker_corners, marker_ids);
  if (marker_ids.empty())
    return false;

  std::vector<cv::Point2f> corners;
  std::vector<int> ids;
  cv::aruco::interpolateCornersCharuco(marker_corners, marker_ids, image,
                                       board_, corners, ids, camera_matrix_,
                                       distortion_);
  // counts_ has one entry per board corner.
  if (static_cast<double>(ids.size()) < thresholds_.min_corner_share_per_frame *
                                            static_cast<double>(counts_.size()))
    return false;

  const double moved = shift(corners, ids);
  if (moved > thresholds_.max_corner_shift_px)
    throw std::runtime_error(
        cv::format("Board corners moved %.2f px from earlier frames, more than "
                   "%.2f px. Keep the camera and board still",
                   moved, thresholds_.max_corner_shift_px));

  for (std::size_t i = 0; i < ids.size(); ++i) {
    sums_[ids[i]] += cv::Point2d(corners[i]);
    ++counts_[ids[i]];
  }
  ++frames_;
  return true;
}

double Estimator::shift(const std::vector<cv::Point2f> &corners,
                        const std::vector<int> &ids) const {
  double squared = 0;
  int compared = 0;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    const int count = counts_[ids[i]];
    if (count == 0)
      continue;
    const cv::Point2d delta = cv::Point2d(corners[i]) - sums_[ids[i]] / count;
    squared += delta.dot(delta);
    ++compared;
  }
  return compared > 0 ? std::sqrt(squared / compared) : 0.0;
}

Result Estimator::solve(const std::string &serial) const {
  if (frames_ == 0)
    throw std::runtime_error("No frame with the board was added");

  // Board corners in meters and their mean pixel, for IDs seen often enough.
  std::vector<cv::Point3d> object;
  std::vector<cv::Point2d> seen;
  for (std::size_t id = 0; id < counts_.size(); ++id) {
    if (counts_[id] == 0 ||
        counts_[id] < thresholds_.min_frame_share_per_corner * frames_)
      continue;
    object.emplace_back(board_->chessboardCorners[id]);
    seen.push_back(sums_[id] / counts_[id]);
  }
  // solvePnP needs at least 4 corners, even if threshold is lower.
  const double needed = std::max(4.0, thresholds_.min_corners_share_to_solve *
                                          static_cast<double>(counts_.size()));
  if (static_cast<double>(object.size()) < needed)
    throw std::runtime_error("Too few board corners seen in enough frames");

  // Board to camera.
  cv::Vec3d rotation_vector;
  cv::Vec3d translation;
  if (!cv::solvePnP(object, seen, camera_matrix_, distortion_, rotation_vector,
                    translation, false, cv::SOLVEPNP_IPPE))
    throw std::runtime_error("Pose estimate failed");
  cv::solvePnPRefineLM(object, seen, camera_matrix_, distortion_,
                       rotation_vector, translation);

  std::vector<cv::Point2d> projected;
  cv::projectPoints(object, rotation_vector, translation, camera_matrix_,
                    distortion_, projected);
  double squared = 0;
  for (std::size_t i = 0; i < seen.size(); ++i) {
    const cv::Point2d delta = seen[i] - projected[i];
    squared += delta.dot(delta);
  }

  // Camera to world: rotation R^T, translation -R^T t.
  cv::Matx33d rotation;
  cv::Rodrigues(rotation_vector, rotation);
  const cv::Matx33d r = rotation.t();
  const cv::Vec3d t = -(r * translation);

  return {cv::Matx44d(r(0, 0), r(0, 1), r(0, 2), t[0], r(1, 0), r(1, 1),
                      r(1, 2), t[1], r(2, 0), r(2, 1), r(2, 2), t[2], 0, 0, 0,
                      1),
          std::sqrt(squared / static_cast<double>(seen.size())),
          serial,
          std::chrono::system_clock::now(),
          frames_,
          static_cast<int>(seen.size())};
}

} // namespace world_pose
