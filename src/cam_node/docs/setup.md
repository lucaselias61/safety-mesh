# Setup

Run commands from `cam_node/`.

## Dependencies

```sh
./scripts/install-deps-ubuntu.sh
```

The installer supports Ubuntu 22.04, 24.04, and 26.04 LTS, using APT and the official RealSense repository.

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

Then create the engine:

```sh
./scripts/build-tensorrt-engine.sh --onnx models/yolo11n.onnx --output models/yolo11n.engine
```

It Generates the GPU-optimized .engine file using trtexec. Regenerate it when the model or GPU/TensorRT environment changes.

For more options and information see [configure inference](inference.md#gpu-acceleration). Engine generation installs no dependencies.

Both setup methods create `bin/cam_node`, so the last build selects the executable. `build.sh` copies the compilation database to `build/compile_commands.json` for the editor; manual builds generate it there directly.
