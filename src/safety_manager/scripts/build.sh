#!/usr/bin/env bash

# Configure and build the safety_manager executable:
# - Release build in build/release.
# - Copy the executable to bin/safety_manager and update the editor compilation database.

# Requires Linux and installed build dependencies.

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

case "${1:-}" in
    "") ;;
    --help|-h) echo "Usage: $0"; exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
esac
[[ "$(uname -s)" == Linux ]] || { echo "Supported build platform: Linux." >&2; exit 1; }
command -v cmake >/dev/null || { echo "Run ./scripts/install-deps-ubuntu.sh first." >&2; exit 1; }

build_dir="build/release"
# APT packages already provide CMake configs in the system search paths.
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "-DSAFETY_MANAGER_OUTPUT_DIR=$project_dir/$build_dir/bin"
# Compile targets with multiple build jobs.
cmake --build "$build_dir" --parallel
mkdir -p bin
cp "$build_dir/bin/safety_manager" bin/safety_manager
# Keep the editor pointed at the latest build.
cp "$build_dir/compile_commands.json" build/compile_commands.json
