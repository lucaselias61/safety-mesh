// CameraStream class manages camera streams and configuration.
#pragma once

#include "camera.hpp"
#include "frames.hpp"
#include "geometry.hpp"
#include "stream.hpp"

#include <filesystem>
#include <librealsense2/rs.hpp>
#include <map>
#include <optional>
#include <string>

class PipelineProfile {
public:
  /// @brief Profile with one matching stream per configured stream.
  /// @throws std::runtime_error if the device offers no match for a stream.
  static PipelineProfile from_streams(const rs2::device &device,
                                      const std::vector<StreamConfig> &streams);

  void add(const StreamProfile &profile) {
    profiles_[profile.id()].push_back(profile);
  }

  const std::map<int, std::vector<StreamProfile>> &get_profiles() const {
    return profiles_;
  }
  std::string to_string() const;

private:
  std::map<int, std::vector<StreamProfile>> profiles_;
};

/** \brief This class manages one camera's pipeline and configuration.
 * \note It wraps the RealSense pipeline and provides methods to control and
 * access the necessary streams.
 */
class Pipeline {
public:
  explicit Pipeline(const Camera &camera,
                    const PipelineProfile &pipeline_profile);

  void start();
  void stop();

  Frames get_frames();
  const Camera &camera() const noexcept;

  /// @brief Calibration state of the active depth and color streams.
  /// @return std::nullopt before start() or when depth or color is disabled.
  const std::optional<geometry::Calibration> &calibration() const noexcept;

private:
  Camera cam_;
  rs2::pipeline pipeline_;
  rs2::config config_;
  PipelineProfile pipeline_profile_;
  std::optional<geometry::Calibration> calibration_;
};
