# Outputs

## Current output

`run` prints enabled stream information, then each detected person's confidence and bounding box:

```text
Detected object with confidence: 0.87
Bounding box [x, y, width, height]: [100, 80, 120, 240]
```

Bounding boxes use color image pixels, with the origin at the top left. No detection means no detection lines for that frame. This is console output; JSON broadcasting is not implemented yet.

## Planned messages

The following examples preserve the intended output: human keypoints and pose in camera frame, along with camera status and timing. These fields are not a current API. Coordinate units, axis conventions, ID lifetime and status values still need to be defined before implementation.

- *State: Detection*

```json
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
```

- *State: No Detection*

```json
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
```
