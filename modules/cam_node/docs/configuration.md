# Configuration

Camera and engine settings share one JSON file. Start from [`serial_number.example.json`](../config/serial_number.example.json).

All fields below are required unless stated otherwise. The example values are not automatic defaults.

## Camera

| Field | Meaning |
| --- | --- |
| `camera.serial` | Nonempty camera serial number, as a string. |
| `camera.enabled` | If false, exit without starting the camera. The engine config is still validated. |
| `camera.streams` | Object of stream names and settings. Names are case insensitive. |

Each stream has:

| Field | Meaning |
| --- | --- |
| `enabled` | If false, skip this stream and its remaining settings. |
| `format` | SDK format name, such as `RGB8` or `Z16`. |
| `width`, `height` | Optional positive integers; omitted values use 0 for SDK selection. |
| `fps` | Positive integer. |

At least one stream must be enabled for an enabled camera. Detection needs an enabled color stream in `RGB8` or `BGR8`. Use profiles reported by the [streams command](usage.md#list-stream-profiles).

## Engine

| Field | Meaning |
| --- | --- |
| `engine.backend` | `opencv_dnn` or `tensorrt`. |
| `engine.model_path` | Existing model file. Relative paths resolve from the working directory. |
| `engine.confidence_threshold` | Person score threshold, greater than 0 and at most 1. |
| `engine.nms_threshold` | Overlap threshold for removing duplicate boxes, greater than 0 and at most 1. |
| `engine.input_size` | Positive integer; square model input size. |
| `engine.precision` | Optional: `fp32` (default), `fp16` or `int8`. Precision TensorRT computes in when it builds an engine from ONNX; `opencv_dnn` accepts only `fp32`. See [inference](inference.md#precision). |

See [inference](inference.md) for model and backend requirements.

## Logging

`LOG_LEVEL` sets the text log verbosity: `debug`, `info` (default), `warn`
or `error`. Messages below it are dropped. Detections are logged at `info`,
missing frames at `warn` and errors at `error`; `debug` and `info` go to
stdout, `warn` and `error` to stderr. An invalid value prints a warning and
uses `info`. It applies to the `run` worker loop and viewer messages; other
commands' output is unaffected.

## Rerun (live debugging)

Optional. Set through environment variables, not the JSON file, so one
config works with and without a viewer. Logging is off when `RERUN_URL` is
unset; an invalid value prints a warning and leaves the camera running
without logging.

| Variable | Meaning |
| --- | --- |
| `RERUN_URL` | Viewer to stream to, e.g. `rerun+http://host.docker.internal:9876/proxy` (see `modules/rerun`). |
| `RERUN_IMAGE_FPS` | Optional. Maximum rate of color and depth images, default `5`; `0` logs no images. Detections and timings are logged every frame. |

Everything is logged under `cams/<serial>/` on the `capture_time` and
`frame` timelines: `color` (image, detections, keypoints), `depth` (depth
image placed relative to color, shown as a point cloud) and `health`
(`wait_ms`, `detect_ms`, `fps`). Build with `-DCAM_NODE_RERUN=OFF` to leave
the Rerun SDK out entirely.
