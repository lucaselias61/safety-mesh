# Cam Node

Its a human detection pipeline that with the help of a RealSense depth camera, can broadcast the detections in camera frame.

## Build

From `cam_node/`, install dependencies for your platform:

```sh
# macOS:
./scripts/install-deps-macos.sh

# Ubuntu Linux:
./scripts/install-deps-linux.sh
```

Then configure and build on either platform:

```sh
./scripts/build.sh
```

macOS requires Homebrew. Linux supports Ubuntu 22.04, 24.04, and 26.04 LTS,
using APT and the official RealSense repository. 

It installs: 
- USB permission rules
- kernel driver

RealSense 2.58 or newer is required.

After source changes, rebuild with:

```sh
cmake --build build --parallel
```

To configure manually on macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build --parallel
```

List connected RealSense camera serial numbers and names:

```sh
./bin/cam_node list
```

Start streaming and detection using a camera config file:

```sh
cp config/camera.json.example config/camera.json
./bin/cam_node run --config config/camera.json
```

or by its serial number:

```sh
./bin/cam_node run --serial 233322245
```

`list` reads only device names and serial numbers
`run` starts streaming and detection when enabled.
Missing cameras or initialization failures return a nonzero exit code.


# Specifications

Written in C++ because the Camera Node depends on low-level camera SDKs, AI inference runtimes, and predictable real-time performance close to the hardware.

# Behaviour 

- **Stream Parsing**  
  Receives the camera stream and prepares it for inference.

- **AI Pose Detection**  
  Detects human pose keypoints in the camera image.

# Outputs

- *State: Detection*

		{
		  "cam_serial": "233322245",
		  "cam_id": "cam_1",
		  "timestamp": "2026-06-23T15:42:10.120Z",
		  "message_id": 18422,
		  "detections": [
			{
			  "person_id": "person_4",
			  "keypoints": {
				"head": [2.44, 1.79, 1.72],
				"neck": [2.45, 1.80, 1.52],
				"torso": [2.46, 1.80, 1.05],
				"hip": [2.47, 1.81, 0.92]
			  },
			  "pose": [2.48, 1.80, 0.0],
			  "confidence": 0.87
			}
		  ],
		  "camera_status": "OK",
		  "calibration_status": "VALID",
		  "fps": 28.7,
		  "latency_ms": 42
		}

- *State: No Detection*
  
		{
		  "cam_serial": "233322245",
		  "cam_id": "cam_1",
		  "timestamp": "2026-06-23T15:42:10.120Z",
		  "message_id": 18423,
		  "detections": [],
		  "camera_status": "OK",
		  "calibration_status": "VALID",
		  "fps": 28.7,
		  "latency_ms": 42
		}

## TensorRT backend

On an NVIDIA GPU system with CUDA and TensorRT 10 or newer installed:

```sh
cmake -S . -B build -DTENSORRT=ON -DTensorRT_ROOT=/path/to/TensorRT
cmake --build build --parallel
```

Build the plan offline on the deployment GPU using NVIDIA's `trtexec`:

```sh
trtexec --onnx=models/yolo11n.onnx --saveEngine=models/yolo11n.engine
```

For a dynamic ONNX input, also supply `--shapes=images:1x3x640x640`
(replace `images` with the model's input name). Set the engine configuration
`backend` to `tensor_rt`, `model_path` to the resulting plan, and `input_size`
to its square image size. The backend expects one FP32 linear NCHW RGB input
and one FP32 output `[1, 84, N]` (standard COCO YOLOv8/YOLO11 detection,
without embedded NMS). It selects people and applies the configured NMS.
Dynamic plans use optimization profile 0, which must support the configured
input shape. FP16 input/output and data-dependent output shapes are unsupported.

Following [NVIDIA's C++ runtime workflow](https://docs.nvidia.com/deeplearning/tensorrt/latest/inference-library/c-api-docs.html),
the backend deserializes a plan, binds reusable device buffers by tensor name,
and uses `enqueueV3` with a CUDA stream. Only load plans from trusted sources;
plans must be compatible with the deployment GPU, platform, and TensorRT version.
Use a separate engine instance for each worker; concurrent calls to the same
instance are unsupported.
