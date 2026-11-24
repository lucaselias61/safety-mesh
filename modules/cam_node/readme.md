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

Copy the settings, set `CAMERA_SERIAL` and `MODEL_PATH`, load them as env vars, then run:

```sh
cp .env.example .env
set -a; . ./.env; set +a
./bin/cam_node run
```

Ubuntu 24.04 LTS is supported by the setup scripts, which install pinned dependency versions, including RealSense 2.58.4.

See [docs](docs/README.md) for setup, configuration and project details.
