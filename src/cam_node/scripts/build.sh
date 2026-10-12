#!/usr/bin/env bash
# Stop on failed commands, unset variables, or failed pipeline steps.
set -euo pipefail

# Run from cam_node regardless of the caller's working directory.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is missing. Run ./scripts/install-deps-macos.sh or ./scripts/install-deps-linux.sh first." >&2
    exit 1
fi

configure_args=(-S . -B build -DCMAKE_BUILD_TYPE=Release)
case "$(uname -s)" in
    Darwin)
        if command -v brew >/dev/null 2>&1; then
            configure_args+=("-DCMAKE_PREFIX_PATH=$(brew --prefix)")
        fi
        ;;
    Linux)
        # APT packages already provide CMake configs in the system search paths.
        ;;
    *)
        echo "Supported build platforms: macOS and Linux." >&2
        exit 1
        ;;
esac

# Configure the build using installed dependencies.
cmake "${configure_args[@]}"

# Compile targets with multiple build jobs.
cmake --build build --parallel
