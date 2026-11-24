# Usage

Run commands from `cam_node/`.

## List cameras

```sh
./bin/cam_node list
```

Reads device names and serial numbers without starting streams.

## List stream profiles

```sh
./bin/cam_node streams --serial 233322245
./bin/cam_node streams --serial 233322245 --filter color --sort fps
```

Filters: `all`, `color`, `depth`, `infrared`. Sort keys: `all`, `fps`, `width`, `height`, `resolution`. Sorting is within each stream ID.

## Run detection

`run` reads its settings from env vars, see [configuration](configuration.md).
Copy [`.env.example`](../.env.example), set `CAMERA_SERIAL` and `MODEL_PATH`
(e.g. `models/yolo11n.onnx` natively), then load it into the shell:

```sh
cp .env.example .env
set -a; . ./.env; set +a
./bin/cam_node run
```

`run` has no options. An empty `CAMERA_SERIAL` uses the only connected camera. Stop with Ctrl+C. SIGTERM also requests shutdown.

Missing cameras, invalid configuration and initialization failures return a nonzero exit code. See [outputs](outputs.md) for detection results.

## Calibrate the camera pose

Finds where the camera sits in the world frame from the ChArUco board in view.
The board file is described in [configuration](configuration.md#charuco-board).

```sh
./bin/cam_node calibrate --serial 233322245 --board config/charuco.json
./bin/cam_node calibrate --serial 233322245 --board config/charuco.json --frames 60 --timeout 60
./bin/cam_node calibrate --serial 233322245 --board config/charuco.json --out calibration/233322245.json
```

- Streams only color, at the camera's highest resolution in RGB8 or BGR8, at
  the lowest fps for it. That allows the longest exposure, so less noise in dim
  light. Needs no camera config.
- `--frames` (default 30) is how many frames with enough of the board are
  averaged. Frames that show too little of the board are skipped.
- `--timeout` (default 30 s) stops with an error if not enough frames show the
  board in time.
- Keep the camera and board still while it runs. It stops with an error if a
  frame's board corners are more than 1 px (RMS) from their mean over the
  earlier frames.

It prints the result, for example:

```text
Camera 233322245 pose in the board frame
  estimated at   2026-11-18T03:12:45Z
  frames used    30
  corners used   24
  reprojection   0.185 px
  position (m)   x 0.2730  y 0.1610  z -0.5470
  camera_to_world (meters)
    [  0.97570  -0.10900   0.19020   0.27300]
    ...
```

`--out` also writes it as JSON, for the manager's camera extrinsics. Content,
shortened:

```json
{
  "serial": "233322245",
  "estimated_at": "2026-11-18T03:12:45Z",
  "frames": 30,
  "corners": 24,
  "reprojection_error_px": 0.185,
  "camera_to_world": [
    [0.9757, -0.109, 0.1902, 0.273],
    ...
  ]
}
```

- `camera_to_world` is 4x4, as rows, in meters. It maps a point in the color
  camera frame (x right, y down, z forward) to the board frame.
- An existing file is replaced.

`position` is the camera's position in the board frame. `camera_to_world` maps a
point in the color camera frame to the board frame. A reprojection error well
above 1 px usually means the board file does not match the printed board or the
board moved. The result is printed only, not saved.

In Docker, stop the service first, since a camera can be opened only once.
Compose mounts `config/charuco.json` at `/opt/cam_node/charuco.json`:

```sh
docker compose down
docker compose run --rm cam-node calibrate --board /opt/cam_node/charuco.json
```

## Record depth and color

`tools/rec_stream.py` records the camera's depth and color streams to a
RealSense recording file. It runs on the host in a Python venv, not in the image.

```sh
python3 -m venv .venv && . .venv/bin/activate
pip install "pyrealsense2==2.58.*"   # match REALSENSE_VERSION in the Dockerfile
python3 tools/rec_stream.py --out recordings/
python3 tools/rec_stream.py --out recordings/ --env .env --duration 30
python3 tools/rec_stream.py --out take1.db3 --serial 233322245
```

- `--out` ending in `.db3` or `.bag` is the file path. Anything else is a
  directory and the file is named `<serial>_<UTC start>.db3`, e.g.
  `233322245_20261124T154210Z.db3`. An existing file is not overwritten.
- `.db3` is rosbag2. The PyPI wheels are built with rosbag2 and record only
  to `.db3`. `.bag` (rosbag1) is for SDK builds without rosbag2.
- `--env` (default `modules/cam_node/.env`) gives the stream settings `run`
  uses: `WIDTH`, `HEIGHT`, `FPS`, `COLOR_*` and `DEPTH_*`. Without the file,
  depth Z16 and color RGB8 at 640x480 and 15 fps are recorded.
- The camera is `--serial`, else `CAMERA_SERIAL` in `--env`, else the only
  connected camera.
- `--duration` (default 0) stops after that many seconds. 0 records until
  Ctrl+C. SIGTERM also stops it.
- The file keeps raw Z16 depth, color, timestamps, intrinsics and depth scale.
  Open it in `realsense-viewer`.
- Not compressed. The defaults write about 23 MB/s.

`pyrealsense2` on PyPI has no macOS build. Stop `cam_node` first, since a
camera can be opened only once.
