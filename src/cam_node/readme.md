# Cam Node

Its a human detection pipeline that with the help of a RealSense depth camera, can broadcast the detections in camera frame.

## Build

To build an independent and fresh config, run from `cam_node/` with CMake 3.24+, Conan 2,
and a C++17 compiler installed:

```sh
./scripts/build.sh
```

If Conan has no default profile yet, run `conan profile detect` once.

The script installs dependencies using `conan.lock`, resets the CMake cache,
and compiles a Release build in parallel.

After source changes, rebuild without reinstalling dependencies:

```sh
cmake --build build --parallel
```

List connected RealSense camera serial numbers and names:

```sh
./bin/cam_node discovery
```

Start the cameras streaming and detection by serial number:

```sh
./bin/cam_node run --serial 233322245
```

Initialization constructs `Camera`objects and reads their intrinsics. It currently exits afterward without streaming.
Missing cameras or initialization failures return a nonzero exit code.

When intentionally changing dependency versions, regenerate and commit the lockfile:

```sh
conan lock create . -s build_type=Release --lockfile-out=conan.lock
```

The parent project can still build this target through `add_subdirectory(src/cam_node)`.
To build only this target using the existing parent build, run:

```sh
cmake --build ../../build/conan-debug --target cam_node
```

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
