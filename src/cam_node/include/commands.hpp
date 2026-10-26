#pragma once

#include "engines/engine.hpp"
#include "pipeline.hpp"

#include <memory>
#include <string>
#include <vector>

void list();

void view(const std::shared_ptr<std::string> &serial);

// Displays one frame pair; call repeatedly on the GUI thread.
void view(const std::shared_ptr<Frames> &frames,
          const std::vector<Detection> &detections = {});

void run(const std::shared_ptr<std::string> &config);

void streams(const std::shared_ptr<std::string> &serial,
             const std::shared_ptr<std::string> &key,
             const std::shared_ptr<std::string> &filter);
