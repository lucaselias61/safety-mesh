#!/usr/bin/env bash
# Stop on failed commands, unset variables, or failed pipeline steps.
set -euo pipefail

# Run from cam_node regardless of the caller's working directory.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if [[ "$(uname -s)" != Linux ]]; then
    echo "Supported build platform: Linux." >&2
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is missing. Run ./scripts/install-deps-ubuntu.sh first." >&2
    exit 1
fi

# APT packages already provide CMake configs in the system search paths.
# Configure the build using installed dependencies.
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile targets with multiple build jobs.
cmake --build build --parallel
