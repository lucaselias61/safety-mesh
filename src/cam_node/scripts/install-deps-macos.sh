#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
    echo "The current platform is not macOS." >&2
    exit 1
fi

if ! command -v brew >/dev/null 2>&1; then
    echo "Install Homebrew first: https://brew.sh" >&2
    exit 1
fi
brew install cmake cli11 fmt librealsense opencv
# Update an existing SDK installation as well as installing a missing one.
brew upgrade librealsense

echo "Dependencies installed. Build with ./scripts/build.sh"
