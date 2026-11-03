#include "deprojection.hpp"

cv::Point3f deproject_pixel_to_point(const cv::Mat &intrinsics,
                                     const cv::Point2f &pixel, float depth) {
  float x = (pixel.x - intrinsics.at<float>(0, 2)) / intrinsics.at<float>(0, 0);
  float y = (pixel.y - intrinsics.at<float>(1, 2)) / intrinsics.at<float>(1, 1);
  return cv::Point3f(x * depth, y * depth, depth);
}
