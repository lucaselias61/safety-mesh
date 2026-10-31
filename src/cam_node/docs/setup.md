# Setup

Run commands from `cam_node/`.

## Dependencies

```sh
./scripts/install-deps-ubuntu.sh
```

The installer supports Ubuntu 22.04, 24.04, and 26.04 LTS, using APT and the official RealSense repository.

It installs the C++ build tools, CMake, CLI11, fmt, nlohmann/json, OpenCV, RealSense SDK and USB permission rules. RealSense 2.58 or newer is required. The script does not install a kernel driver.

## Build

```sh
./scripts/build.sh
```

Or configure manually with CMake 3.20 or newer and a C++17 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

After source changes, rebuild with:

```sh
cmake --build build --parallel
```

The executable is written to `bin/cam_node`. For GPU builds, see [inference](inference.md).
