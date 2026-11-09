# Inference

Multiple models were tested to compare requirements and results. The models were run on the CPU first for validating the output tensor and the general end-to-end flow.

## CPU implementation

[OpenCV DNN](https://opencv.org/opencv-dnn-module/) loads ONNX models and runs inference on the CPU. The example uses `models/yolo11n.onnx` with `backend` set to `opencv_dnn`.

## Model requirements

Both backends accept `RGB8` or `BGR8` color frames, normalize pixels to 0–1, pad the image to a square and resize to `input_size`.

Use a standard COCO YOLOv8/YOLO11 detection export with output `[1, 84, N]`, without embedded NMS. The engines select people (class 0) and apply the configured confidence and NMS thresholds. Pose models are not supported.

## GPU acceleration

For a tar installation, add `--tensorrt-root /path/to/TensorRT` to the build command.

Set `backend` to `tensorrt`, `model_path` to the ONNX model, and `input_size` to its square image size. On first run, `cam_node` builds an engine for the current GPU and caches it in `.engines/` next to the model, with a `.json` file describing it. Later runs reuse it. The cache key covers the model contents, input size, GPU, CUDA and TensorRT versions, so changing any of them builds a new engine. The first build can take several minutes; Processes sharing the directory wait for each other, so each engine is built once.

To build ahead of time instead, run `scripts/build-tensorrt-engine.sh` on the deployment GPU and set `model_path` to the resulting plan. For dynamic inputs, add `--shapes images:1x3x640x640`, using the model's input name. For a tar installation, set `TRTEXEC=/path/to/TensorRT/bin/trtexec`. The script refuses existing outputs and writes model hash, GPU, driver and TensorRT details to `.engine.meta.txt`. Paths that do not end in `.onnx` are loaded as plans.

The backend expects one FP32 linear NCHW RGB input and one FP32 output `[1, 84, N]`. Dynamic plans use optimization profile 0, which must support the configured input shape. FP16 input/output and data-dependent output shapes are unsupported.

The runtime deserializes a plan, binds reusable device buffers by tensor name and uses `enqueueV3` with a CUDA stream. Only load plans from trusted sources; plans must be compatible with the deployment GPU, platform and TensorRT version. Use a separate engine instance for each worker; concurrent calls to the same instance are unsupported.

## Compared to NVIDIA's documentation

What matches
- ***Linking***: it uses NvInfer.h and NvOnnxParser.h, and links nvinfer, nvonnxparser and cudart.
- ***Call order***: createInferBuilder → createNetworkV2(0) → createParser → parseFromFile with error reporting → createBuilderConfig → buildSerializedNetwork.
- ***Dynamic inputs***: it sets min/opt/max dimensions and adds the optimization profile, which NVIDIA requires for dynamic shapes.
- ***Lifetimes***: the parser stays alive until the build finishes, and all builder objects are freed before the runtime is created.
- ***Cache key***: it covers the TensorRT version and compute capability, the two things NVIDIA says make a plan fail to load. Including the GPU UUID and driver is stricter than required, but safe.

Where it falls short
1. ***Building at startup***: NVIDIA says engine building is "intended as an offline process", while this builds on first run. It's a reasonable tradeoff since scripts/build-tensorrt-engine.sh is still there, but production should prebuild to avoid a startup that takes minutes.
2. ***Library path, not documented***: building loads libnvinfer_builder_resource_*.so from the system library path, such as LD_LIBRARY_PATH. Tar installs need this, but docs/inference.md only mentions --tensorrt-root. It also needs the Full package, as Lean and Dispatch only run engines.
3. ***TensorRT 10 is still allowed***: tensorrt_common.hpp accepts 10+, but the docs only say networks are strongly typed by default in 11. On 10, createNetworkV2(0) may behave differently, so either require 11 or test on 10. CUDA 13.4 and driver r580+ are the requirements for the 11.4.1 packages.
4. ***Cached engines are trusted as-is***: NVIDIA warns that engines are executable and should only come from trusted sources. Anything that can write to models/.engines/ could plant one, and the check is only a non-cryptographic hash used in the filename. Keep that directory writable only by the service.
5. ***No memory limit (optional)***: without setMemoryPoolLimit, the build workspace defaults to all of the GPU's memory. This matters if the GPU is shared while building.