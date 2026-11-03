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

Ubuntu 24.04 LTS is supported by the setup scripts, which install pinned dependency versions, including RealSense 2.58.4.

See [docs](docs/README.md) for setup, configuration and project details.
