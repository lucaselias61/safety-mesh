#!/usr/bin/env bash

# Check the environment and install GPU dependencies:
# - CUDA toolkit.
# - TensorRT development libraries and headers.
# - TensorRT command-line tools, including trtexec.
# - ONNX parser development files.

# Requires Ubuntu 24.04 or newer (x86-64), a working NVIDIA driver,
# and NVIDIA's CUDA APT repository for the installed Ubuntu release.

set -euo pipefail

usage() {
    echo "Usage: $0 --cuda-version MAJOR-MINOR --tensorrt-version APT_VERSION"
    echo "Requires NVIDIA's CUDA APT repository and a working NVIDIA driver."
}
cuda_version=""
tensorrt_version=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cuda-version|--tensorrt-version)
            [[ $# -ge 2 && -n "$2" ]] || { usage >&2; exit 1; }
            if [[ "$1" == --cuda-version ]]; then cuda_version="$2"; else tensorrt_version="$2"; fi
            shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) usage >&2; exit 1 ;;
    esac
done
[[ "$cuda_version" =~ ^[0-9]+-[0-9]+$ && "$tensorrt_version" =~ ^10\. ]] || {
    echo "Specify a CUDA release and exact TensorRT 10 APT version." >&2; usage >&2; exit 1;
}
[[ "$(uname -s)" == Linux ]] || { echo "Requires Ubuntu Linux." >&2; exit 1; }
. /etc/os-release
if [[ "${ID:-}" != ubuntu || "$(uname -m)" != x86_64 || ! "${VERSION_ID:-}" =~ ^[0-9]+\.[0-9]+$ ]] ||
    ! dpkg --compare-versions "$VERSION_ID" ge 24.04; then
    echo "GPU installer requires Ubuntu 24.04 or newer, x86-64." >&2
    exit 1
fi
if [[ "$VERSION_ID" != 24.04 ]]; then
    echo "Ubuntu $VERSION_ID: CUDA/TensorRT packages must support this release; deployment validation is required." >&2
fi
command -v nvidia-smi >/dev/null && nvidia-smi >/dev/null || {
    echo "Install a compatible NVIDIA driver first." >&2; exit 1;
}
root_cmd=(env)
if [[ "$(id -u)" != 0 ]]; then root_cmd=(sudo); fi
"${root_cmd[@]}" apt-get update
packages=("cuda-toolkit-$cuda_version")
for package in libnvinfer-dev libnvinfer-headers-dev libnvinfer-bin libnvonnxparsers-dev; do
    packages+=("$package=$tensorrt_version")
done
# Resolve everything before installing. Never install the driver-bearing 'cuda' package.
"${root_cmd[@]}" apt-get --simulate install --no-install-recommends "${packages[@]}"
"${root_cmd[@]}" apt-get install -y --no-install-recommends "${packages[@]}"
echo "GPU dependencies installed. Build with ./scripts/build.sh --tensorrt"
