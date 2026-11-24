# Configuration

`cam_node run` reads its camera and engine settings from env vars. Compose passes
them from `.env`. Start from [`.env.example`](../.env.example). Natively, load it
into the shell with `set -a; . ./.env; set +a`.

All variables below are required unless stated otherwise. An empty value counts
as unset. A missing or invalid one stops `run` with its name.

## Camera

| Variable | Meaning |
| --- | --- |
| `CAMERA_SERIAL` | Optional. Camera serial. Empty: the only connected camera. |
| `CAMERA_ENABLED` | `true` or `false`. `false` exits without opening the camera. |
| `COLOR_FORMAT`, `DEPTH_FORMAT` | SDK format name, such as `RGB8` or `Z16`. Color and depth always both stream. |
| `WIDTH`, `HEIGHT`, `FPS` | Positive integers. Color and depth always share them. |

Detection needs color in `RGB8` or `BGR8`. Use profiles reported by the
[streams command](usage.md#list-stream-profiles).

## Engine

| Variable | Meaning |
| --- | --- |
| `ENGINE_BACKEND` | Optional. `opencv_dnn` or `tensorrt`. Default: `tensorrt` in a TensorRT build, else `opencv_dnn`. |
| `MODEL_PATH` | Existing model file. Relative paths resolve from the working directory. |
| `CONFIDENCE_THRESHOLD` | Person score threshold, greater than 0 and at most 1. |
| `NMS_THRESHOLD` | Overlap threshold for removing duplicate boxes, greater than 0 and at most 1. |
| `INPUT_SIZE` | Positive integer, square model input size. |
| `PRECISION` | Optional: `fp32` (default), `fp16` or `int8`. Precision TensorRT computes in when it builds an engine from ONNX; `opencv_dnn` accepts only `fp32`. See [inference](inference.md#precision). |

See [inference](inference.md) for model and backend requirements.

## ChArUco board

[`charuco.json`](../config/charuco.json) describes the printed board used to find a camera's pose in the world frame. One file serves all cameras. All fields are required.

| Field | Meaning |
| --- | --- |
| `rows` | Squares along the board's Y axis, at least 2. |
| `columns` | Squares along the board's X axis, at least 2. |
| `square_size_m` | Side of one square, in meters. |
| `marker_size_m` | Side of one ArUco marker, in meters. Smaller than the square. |
| `dictionary` | ArUco dictionary, `DICT_<N>X<N>_<count>` with N from 4 to 7 and count 50, 100, 250 or 1000. |

- The file must match the real board exactly. Accurately measure the squares.
- Generate the board with OpenCV 4.6 or newer. Older versions lay out boards with an even number of rows differently. An odd number of rows avoids the difference.
- The values in the repository are placeholders until the real board is printed.

The board frame is the world frame: origin at the board's top-left corner, X along the columns, Y along the rows, Z into the board.

## Logging

`LOG_LEVEL` sets the text log verbosity: `debug`, `info` (default), `warn`
or `error`. Messages below it are dropped. Detections are logged at `info`,
missing frames at `warn` and errors at `error`; `debug` and `info` go to
stdout, `warn` and `error` to stderr. An invalid value prints a warning and
uses `info`. It applies to the `run` worker loop and viewer messages; other
commands' output is unaffected.

## Rerun (live debugging)

Optional, like the settings above set through env vars. Logging is off when `RERUN_URL` is
unset; an invalid value prints a warning and leaves the camera running
without logging.

| Variable | Meaning |
| --- | --- |
| `RERUN_URL` | Viewer to stream to, e.g. `rerun+http://host.docker.internal:9876/proxy` (see `modules/rerun`). |
| `RERUN_IMAGE_FPS` | Optional. Maximum rate of color and depth images, default `5`; `0` logs no images. Detections and timings are logged every frame. |

Everything is logged under `cams/<serial>/` on the `capture_time` and
`frame` timelines: `color` (image, detections, keypoints, skeleton), `depth`
(depth image placed relative to color, shown as a point cloud), `space` (3D
keypoints and skeleton in the color camera frame, when depth is enabled) and
`health` (`wait_ms`, `detect_ms`, `deproject_ms`, `log_ms`, `age_ms`, `fps`). `wait_ms`
is time blocked waiting for frames, `detect_ms` is inference, `deproject_ms` is
placing keypoints in 3D from depth, `log_ms` is output time of the previous
frame and `age_ms` is color capture to 3D keypoints ready. Global time is mandatory. cam_node turns
it on for every streaming sensor at start and stops if a sensor does not support
it, if frames do not arrive in global time, or if a later frame is in another
domain. Build with `-DCAM_NODE_RERUN=OFF` to leave
the Rerun SDK out entirely.
