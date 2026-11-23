# Manager

Handles the central coordination and safety decision pipeline.

# Specifications

Written in C++ because the Manager needs reliable state handling, safe concurrency, strict message validation, and deterministic safety decisions without memory-safety risks.

# Behaviour

- **Camera's Signal Health Check** 
  Receives heartbeat messages from all Camera Nodes and checks their status, latency, FPS, and calibration validity.

- **Human Pose Collection**  
  Receives detected poses in world coordinates from all Camera Nodes.

- **Robot Pose Collection**  
  Receives robot pose, position, speed, and operating state from the robots interfaces (SDK) or PLC layer.

- **Global Perception**
  Builds one shared view from all Camera Nodes. It merges detections of the same person using world-frame pose and timestamp similarity, removes duplicates, tracks humans across cameras, and maintains the latest known human positions in the world frame.

- **Distance Calculation**  
  Calculates the distance between each detected human and the active robots safety bubbles.

- **Safety Decision**  
  Evaluates whether each robot is allowed to run, should enter warning state, or must stop.

- **State Publisher**  
  Publishes the final robot safety state to the PLC. The PLC remains responsible for enforcing the robot stop.

# Inputs
[[Camera Node#Outputs]]
## Outputs

- *Message: Robot 1*

		{
			  "robot_serial": "22344555",
			  "robot_id": "robot_1"
			  "timestamp": "2026-06-23T15:42:10.140Z",
			  "state": "RUN",
			  "nearest_person_id": "person_4",
			  "distance_to_person_m": 0.42,
			  "source_cameras": ["cam_1", "cam_2"],
			  "orchestrator_status": "OK",
			  "latency_ms": 20,
		}

- *Message: Robot 2*

		{
			  "robot_serial": "22344555",
			  "robot_id": "robot_2"
			  "timestamp": "2026-06-23T15:42:10.140Z",
			  "state": "STOP",
			  "nearest_person_id": "person_2",
			  "distance_to_person_m": 0.20,
			  "source_cameras": ["cam_3"],
			  "orchestrator_status": "OK",
			  "latency_ms": 20,
		}

# Run

```sh
./scripts/build.sh                 # RERUN=OFF builds without the viewer
set -a; . ./.env; set +a           # LOG_LEVEL, RERUN_URL
./bin/manager run --config config/manager.json
```

`run` loads the config, logs the camera poses once, then polls every robot's
PLC (`--rate`, default 10 per second) until Ctrl+C. ADS errors warn once and
retry every poll.

`config/manager.json`:

- `cameras`: `{id, serial}`. Each loads `config/cameras/<serial>.json`, written
  by `cam_node calibrate --out` (or cam_node's `compose.calibrate.yaml`). A
  listed camera without a valid file stops the start.
- `robots`: `{id, model, base_to_world, plc}`. `base_to_world` is the robot
  base pose in the world (board) frame, 4x4 as rows, meters. `model` loads
  `config/robots/<model>.json`.
- The PLC sends the TCP in millimeters and joints in degrees.
- Joint positions need every DH value in the model file. While any is `null`,
  only the TCP is logged.

With `RERUN_URL` set, the camera poses go to `cams/<serial>` in the shared
recording, so cam_node's images, point clouds and keypoints appear in the world
frame. Robots go to `robots/<id>` (`joints`, `links`, `tcp`). The viewer layout's
World tab shows both.
