#!/usr/bin/env bash

# Configure and build the cam_node executable:
# - CPU backend by default, TensorRT with --tensorrt.
# - Separate build directories for each backend.
# - Copy the executable to bin/cam_node and update the editor compilation database.

# Requires Linux and installed build dependencies. TensorRT builds also need CUDA and TensorRT.

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

backend=cpu
tensorrt_root="${TensorRT_ROOT:-}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tensorrt) backend=tensorrt; shift ;;
        --tensorrt-root)
            [[ $# -ge 2 && -n "$2" ]] || { echo "Missing TensorRT path" >&2; exit 1; }
            tensorrt_root="$2"; shift 2 ;;
        --help|-h) echo "Usage: $0 [--tensorrt] [--tensorrt-root PATH]"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done
[[ "$backend" == tensorrt || -z "$tensorrt_root" ]] || {
    echo "TensorRT root requires --tensorrt" >&2; exit 1;
}
[[ "$(uname -s)" == Linux ]] || { echo "Supported build platform: Linux." >&2; exit 1; }
command -v cmake >/dev/null || { echo "Run ./scripts/install-deps-ubuntu.sh first." >&2; exit 1; }

build_dir="build/$backend"
options=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DTENSORRT=OFF)
if [[ "$backend" == tensorrt ]]; then
    options=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DTENSORRT=ON)
    [[ -z "$tensorrt_root" ]] || options+=("-DTensorRT_ROOT=$tensorrt_root")
fi
# APT packages already provide CMake configs in the system search paths.
# Configure the build using installed dependencies.
cmake -S . -B "$build_dir" "${options[@]}" "-DCAM_NODE_OUTPUT_DIR=$project_dir/$build_dir/bin"
# Compile targets with multiple build jobs.
cmake --build "$build_dir" --parallel
mkdir -p bin
cp "$build_dir/bin/cam_node" bin/cam_node
# Keep the editor pointed at the most recently built backend.
cp "$build_dir/compile_commands.json" build/compile_commands.json
