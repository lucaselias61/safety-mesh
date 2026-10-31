#!/usr/bin/env bash
set -euo pipefail

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
    ubuntu:22.04|ubuntu:24.04|ubuntu:26.04) ;;
    *) echo "Supported Linux distributions: Ubuntu 22.04, 24.04, and 26.04 LTS." >&2; exit 1 ;;
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
"${root_cmd[@]}" apt-get install -y build-essential cmake libcli11-dev libfmt-dev \
    libopencv-dev librealsense2-dev librealsense2-udev-rules nlohmann-json3-dev
sdk_version="$(dpkg-query -W -f='${Version}' librealsense2-dev)"
if ! dpkg --compare-versions "$sdk_version" ge 2.58; then
    echo "RealSense $sdk_version was installed, but this project requires 2.58 or newer." >&2
    echo "Build the required SDK from source if it is unavailable for this Ubuntu release." >&2
    exit 1
fi

echo "Dependencies installed. Build with ./scripts/build.sh"
