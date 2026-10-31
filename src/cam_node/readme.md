# Cam Node

A human detection pipeline using a RealSense depth camera. The goal is to broadcast detections in camera frame.

Currently, it detects people in the color stream and prints their bounding boxes and confidence.

## Quick start

From `cam_node/`, install dependencies and build:

```sh
./scripts/install-deps-ubuntu.sh
./scripts/build.sh
```

List connected cameras:

```sh
./bin/cam_node list
```

Copy the configuration, set `camera.serial` to your camera's serial number, then run:

```sh
cp config/cam_node.example.json config/cam_node.json
./bin/cam_node run --config config/cam_node.json
```

Ubuntu 22.04, 24.04, and 26.04 LTS are supported by the setup scripts. RealSense 2.58 or newer is required.

See [docs](docs/README.md) for setup, configuration and project details.
