#!/usr/bin/env bash

# Check the environment and install Ubuntu build dependencies:
# - C++ build tools, CMake, CLI11, fmt and nlohmann/json.
# - OpenCV libraries.
# - RealSense SDK and USB permission rules.

# Requires Ubuntu 24.04 and root or sudo access.
# Package versions are pinned; update them together when upgrading.

set -euo pipefail

# Pinned for Ubuntu 24.04 (noble) and the official RealSense repository.
realsense_version=2.58.4-0~realsense.19927
packages=(
    build-essential=12.10ubuntu1
    cmake=3.28.3-1build7
    libcli11-dev=2.4.1+ds-1
    libfmt-dev=9.1.0+ds1-2
    nlohmann-json3-dev=3.11.3-1
    libopencv-dev=4.6.0+dfsg-13.1ubuntu1
    "librealsense2=$realsense_version"
    "librealsense2-dev=$realsense_version"
    "librealsense2-udev-rules=$realsense_version"
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
"${root_cmd[@]}" apt-get install -y ca-certificates curl gnupg software-properties-common
"${root_cmd[@]}" add-apt-repository -y universe

# Official repository setup:
# https://github.com/realsenseai/librealsense/blob/master/doc/distribution_linux.md
key_dir="$(mktemp -d)"
trap 'rm -rf "$key_dir"' EXIT
curl --fail --silent --show-error --location \
    https://librealsense.realsenseai.com/Debian/librealsenseai.asc \
    -o "$key_dir/librealsenseai.asc"
gpg --batch --dearmor --output "$key_dir/librealsenseai.gpg" "$key_dir/librealsenseai.asc"
"${root_cmd[@]}" install -d -m 0755 /etc/apt/keyrings
"${root_cmd[@]}" install -m 0644 "$key_dir/librealsenseai.gpg" /etc/apt/keyrings/librealsenseai.gpg
printf 'deb [signed-by=/etc/apt/keyrings/librealsenseai.gpg] https://librealsense.realsenseai.com/Debian/apt-repo %s main\n' \
    "$VERSION_CODENAME" | "${root_cmd[@]}" tee /etc/apt/sources.list.d/librealsense.list >/dev/null

"${root_cmd[@]}" apt-get update
"${root_cmd[@]}" apt-get install -y "${packages[@]}"

echo "Dependencies installed. Build with ./scripts/build.sh"
