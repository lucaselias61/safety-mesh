#!/usr/bin/env bash

# Configure and build the manager executable:
# - Release build in build/release.
# - RERUN=OFF (shell or .env) builds without the Rerun viewer.
# - Copy the executable to bin/manager and update the editor compilation database.

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

# RERUN=OFF leaves the Rerun SDK out. Shell value first, then .env, then ON.
rerun="${RERUN:-}"
if [[ -z "$rerun" && -f .env ]]; then
    rerun="$(sed -n 's/^RERUN=//p' .env | tail -n 1)"
fi
rerun="${rerun:-ON}"

build_dir="build/release"
# APT packages already provide CMake configs in the system search paths.
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "-DMANAGER_OUTPUT_DIR=$project_dir/$build_dir/bin" "-DMANAGER_RERUN=$rerun"
# Compile targets with multiple build jobs.
cmake --build "$build_dir" --parallel
mkdir -p bin
cp "$build_dir/bin/manager" bin/manager
# Keep the editor pointed at the latest build.
cp "$build_dir/compile_commands.json" build/compile_commands.json
