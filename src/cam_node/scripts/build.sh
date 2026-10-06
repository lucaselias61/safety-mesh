#!/usr/bin/env bash
# Stop on failed commands, unset variables, or failed pipeline steps.
set -euo pipefail

# Run from cam_node regardless of the caller's working directory.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

# Install locked Release dependencies into build/, compiling missing binaries.
conan install . --lockfile=conan.lock --output-folder=build \
  -s build_type=Release --build=missing

# Reset the CMake cache and configure a Release build using Conan's toolchain.
cmake --fresh -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Compile targets with multiple build jobs.
cmake --build build --parallel
