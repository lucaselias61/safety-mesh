# Setup

Run commands from `cam_node/`.

For container deployment, see [Docker setup](docker.md). It includes the GPU libraries
and builds the engine on the target PC at startup.

## Dependencies

```sh
./scripts/install-deps-ubuntu.sh
```

The installer accepts Ubuntu 24.04 or newer, using APT and the official RealSense repository. On 24.04, package versions, including RealSense 2.58.4, are pinned in the script. On newer releases, it uses available package versions, checks for RealSense SDK 2.58 or newer, and simulates dependency installation before installing the build dependencies. APT candidates can change between deployments.

Accepting a newer Ubuntu version does not establish compatibility. Installation requires the RealSense repository to publish packages for that release's codename; the installer stops if it does not. Validate installation, camera access, and inference on the target machine before deployment. Do not substitute an older Ubuntu repository on a newer release.

It installs the C++ build tools, CMake, CLI11, fmt, nlohmann/json, OpenCV, RealSense SDK and USB permission rules. RealSense 2.58 or newer is required. The script does not install a kernel driver.

## Build the CPU executable

```sh
./scripts/build.sh # builds the CPU executable using `build/cpu/`
```

or manually with CMake 3.20 or newer and a C++17 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT=OFF
cmake --build build --parallel
```

## Build the TensorRT executable

First you'll have to install the GPU Drivers and Toolkits. see [gpu-setup](gpu-setup.md).

```sh
./scripts/build.sh --tensorrt # TensorRT executable using `build/tensorrt/`
```

Or configure manually with CMake 3.20 or newer and a C++17 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT=ON
cmake --build build --parallel
```

Set `engine.model_path` to the ONNX model, for example `models/yolo11n.onnx`. The first run builds a GPU-optimized engine and caches it; see [configure inference](inference.md#gpu-acceleration).

Both setup methods create `bin/cam_node`, so the last build selects the executable. `build.sh` copies the compilation database to `build/compile_commands.json` for the editor; manual builds generate it there directly.
