# Configuration

Camera and engine settings share one JSON file. Start from [cam_node.example.json](../config/cam_node.example.json).

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
| `engine.backend` | `opencv_dnn` or `tensor_rt`. |
| `engine.model_path` | Existing model file. Relative paths resolve from the working directory. |
| `engine.confidence_threshold` | Person score threshold, greater than 0 and at most 1. |
| `engine.nms_threshold` | Overlap threshold for removing duplicate boxes, greater than 0 and at most 1. |
| `engine.input_size` | Positive integer; square model input size. |

See [inference](inference.md) for model and backend requirements.
