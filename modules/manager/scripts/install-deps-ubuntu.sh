#!/usr/bin/env bash

# Check the environment and install Ubuntu build dependencies:
# - C++ build tools, CMake, CLI11, fmt, nlohmann/json and Protobuf.

# Requires Ubuntu 24.04 and root or sudo access.
# Package versions are pinned; update them together when upgrading.

set -euo pipefail

# Pinned for Ubuntu 24.04 (noble).
packages=(
    build-essential=12.10ubuntu1
    cmake=3.28.3-1build7
    libcli11-dev=2.4.1+ds-1
    libfmt-dev=9.1.0+ds1-2
    nlohmann-json3-dev=3.11.3-1
    libprotobuf-dev=3.21.12-8.2ubuntu0.4
    protobuf-compiler=3.21.12-8.2ubuntu0.4
)

if [[ "$(uname -s)" != Linux ]]; then
    echo "The current platform is not Linux." >&2
    exit 1
fi

if [[ ! -r /etc/os-release ]]; then
    echo "Cannot identify the Linux distribution: /etc/os-release is missing." >&2
    exit 1
fi
. /etc/os-release
case "${ID:-}:${VERSION_ID:-}" in
    ubuntu:24.04) ;;
    *) echo "Supported Linux distribution: Ubuntu 24.04 LTS." >&2; exit 1 ;;
esac

root_cmd=()
if [[ "$(id -u)" != 0 ]]; then
    if ! command -v sudo >/dev/null 2>&1; then
        echo "Install sudo or run this installer as root." >&2
        exit 1
    fi
    root_cmd=(sudo)
fi
# Use a nonempty command array for compatibility with Bash 3 and set -u.
root_cmd+=(env)
"${root_cmd[@]}" apt-get update
"${root_cmd[@]}" apt-get install -y software-properties-common
"${root_cmd[@]}" add-apt-repository -y universe
"${root_cmd[@]}" apt-get update
"${root_cmd[@]}" apt-get install -y "${packages[@]}"

echo "Dependencies installed. Build with ./scripts/build.sh"
