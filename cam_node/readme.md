Handles the local perception pipeline for one 3D camera.

[[cam_node.svg]]
# Specifications

Written in C++ because the Camera Node depends on low-level camera SDKs, OpenCV, AI inference runtimes, and predictable real-time performance close to the hardware.

# Behaviour 

- **Stream Parsing**  
  Receives the camera stream and prepares it for inference.

- **AI Pose Detection**  
  Detects human pose keypoints in the camera image.

- **Pose in Camera Frame**  
  Reconstructs the detected pose in the camera coordinate frame using the camera intrinsics and depth data.

- **World-Frame Transform**  
  Transforms the camera-frame pose into the shared world frame using the calibrated camera extrinsics.

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
			  "keypoints_world": {
				"head": [2.44, 1.79, 1.72],
				"neck": [2.45, 1.80, 1.52],
				"torso": [2.46, 1.80, 1.05],
				"hip": [2.47, 1.81, 0.92]
			  },
			  "pose_world": [2.48, 1.80, 0.0],
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
