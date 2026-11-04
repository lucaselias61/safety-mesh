#!/usr/bin/env bash

# Check the environment and install Ubuntu build dependencies:
# - C++ build tools, CMake, CLI11, fmt and nlohmann/json.
# - OpenCV libraries.
# - RealSense SDK and USB permission rules.

# Requires Ubuntu 24.04 or newer and root or sudo access.
# Package versions are pinned on 24.04; newer releases use APT candidates.

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
if [[ "${ID:-}" != ubuntu || ! "${VERSION_ID:-}" =~ ^[0-9]+\.[0-9]+$ ]] ||
    ! dpkg --compare-versions "$VERSION_ID" ge 24.04; then
    echo "Requires Ubuntu 24.04 or newer." >&2
    exit 1
fi
if [[ -z "${VERSION_CODENAME:-}" ]]; then
    echo "Cannot identify the Ubuntu repository codename." >&2
    exit 1
fi
if [[ "$VERSION_ID" != 24.04 ]]; then
    # Do not request Noble package versions from a newer Ubuntu archive.
    for index in "${!packages[@]}"; do
        packages[$index]="${packages[$index]%%=*}"
    done
    echo "Ubuntu $VERSION_ID: using available packages; this release needs deployment validation." >&2
fi

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
if [[ "$VERSION_ID" != 24.04 ]]; then
    realsense_candidate="$(LC_ALL=C apt-cache policy librealsense2-dev | awk '/Candidate:/ {print $2; exit}')"
    if [[ -z "$realsense_candidate" || "$realsense_candidate" == '(none)' ]] ||
        ! dpkg --compare-versions "$realsense_candidate" ge 2.58; then
        echo "Requires a RealSense SDK >= 2.58 package for Ubuntu $VERSION_ID ($VERSION_CODENAME)." >&2
        exit 1
    fi
fi
"${root_cmd[@]}" apt-get --simulate install "${packages[@]}"
"${root_cmd[@]}" apt-get install -y "${packages[@]}"

echo "Dependencies installed. Build with ./scripts/build.sh"
