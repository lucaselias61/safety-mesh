#pragma once

#include <librealsense2/h/rs_types.h>
#include <opencv2/core.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace cv::aruco {

class CharucoBoard;
class Dictionary;

} // namespace cv::aruco

/// Camera pose in the world frame, from a fixed ChArUco board.
/// - Board frame is the world frame.
/// - Board's top-left corner is the origin. X along columns, Y along rows, Z
/// into the board.
/// - Camera and board fixed.
namespace world_pose {

/// Real ChArUco board. Must match exactly.
struct Board {
  int rows;               // Squares along Y.
  int columns;            // Squares along X.
  float square_size;      // Meters.
  float marker_size;      // Meters. Smaller than square_size.
  std::string dictionary; // DICT_<N>X<N>_<count>, N 4 to 7, count 50 to 1000.
};

/// @brief Loads a board from JSON.
/// @details Fields: rows, columns, square_size_m, marker_size_m, dictionary.
/// @throws std::runtime_error if the file cannot be read or is invalid.
Board load_board(const std::string &path);

struct Thresholds {
  // Share of corners a frame must show.
  double min_corner_share_per_frame = 0.75;
  // Share of frames a corner must be seen in.
  double min_frame_share_per_corner = 0.9;
  // Share of corners needed to solve.
  double min_corners_share_to_solve = 0.9;
};

struct Result {
  /// 4x4, meters. Maps a point in the color camera frame to the world frame.
  cv::Matx44d camera_to_world;
  /// RMS distance in pixels between seen and projected corners.
  double reprojection_error;
  std::string serial;
  std::chrono::system_clock::time_point timestamp;
  int frames;  // Frames used.
  int corners; // Corners used.
};

/// Collects board corners frame by frame, then solves the pose once.
class Estimator {
public:
  /// @param color_intr Intrinsics of the color stream the frames come from.
  /// @throws std::invalid_argument board is invalid or the distortion
  /// model is not none or Brown-Conrady.
  Estimator(const Board &board, const rs2_intrinsics &color_intr,
            const Thresholds &thresholds = {});

  /// @brief Adds board corners seen in frame.
  /// @param image 8-bit gray or color image at the intrinsics resolution.
  /// @return false if frame skipped for having few corners.
  /// @throws std::invalid_argument if invalid image format or size.
  bool add(const cv::Mat &image);

  /// Frames used so far.
  int frames() const noexcept { return frames_; }

  /// @brief Solves the pose from the corners averaged over the used frames.
  /// @throws std::runtime_error if no frame was used or too few corners were
  /// seen in enough frames.
  Result solve(const std::string &serial) const;

private:
  Thresholds thresholds_;
  cv::Size size_;
  cv::Matx33d camera_matrix_;
  cv::Vec<double, 5> distortion_; // k1 k2 p1 p2 k3.
  cv::Ptr<cv::aruco::Dictionary> dictionary_;
  cv::Ptr<cv::aruco::CharucoBoard> board_;
  std::vector<cv::Point2d> sums_; // Pixel sum per corner ID.
  std::vector<int> counts_;       // Frames each corner ID was seen in.
  int frames_ = 0;
};

} // namespace world_pose
