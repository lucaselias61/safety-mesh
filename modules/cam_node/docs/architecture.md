# Architecture

Written in C++ because the Camera Node depends on low-level camera SDKs, AI inference runtimes, and predictable real-time performance close to the hardware.

## Flow

```text
CLI → configuration → Camera → Pipeline → Worker → Engine → detections
```

- **Camera** discovers RealSense devices and provides their serial numbers, intrinsics and stream profiles.
- **Pipeline** starts the configured streams and receives frames.
- **Worker** passes color frames to the engine and hands detections to the **Logger**. Stops after five consecutive missing frames or a shutdown request.
- **Engine** detects people using OpenCV DNN or TensorRT through a common interface.
- **Logger** decides what is logged and where: text at `LOG_LEVEL`, and, with `RERUN_URL`, the Rerun viewer through `ViewerSink`, which converts camera types to Rerun archetypes and sends them over the shared `ViewerConnection` from `modules/rerun`.

Declarations are in `include/`, implementations in `src/`, and commands in `src/cli/`.

## Intended behavior

- **Stream parsing:** receives the camera stream and prepares it for inference.
- **AI pose detection:** detects human pose keypoints in the camera image.
- **Camera frame output:** uses depth to locate detections in 3D and broadcasts them.

Person bounding box detection is implemented. Pose keypoints and broadcasting are planned. Camera geometry helpers (deproject, project, transform, color-to-depth pixel) exist in `src/geometry.cpp`, but are not connected to the worker yet. See [outputs](outputs.md).
