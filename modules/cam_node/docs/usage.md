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

Copy [the example configuration](../config/serial_number.example.json) and set your camera serial:

```sh
cp config/serial_number.example.json config/233322245.json
./bin/cam_node run --config config/233322245.json
```

`run` requires `--config`; it has no `--serial` option. Stop with Ctrl+C. SIGTERM also requests shutdown.

Missing cameras, invalid configuration and initialization failures return a nonzero exit code. See [outputs](outputs.md) for detection results.

## Calibrate the camera pose

Finds where the camera sits in the world frame from the ChArUco board in view.
The board file is described in [configuration](configuration.md#charuco-board).

```sh
./bin/cam_node calibrate --config config/233322245.json --board config/charuco.json
./bin/cam_node calibrate --config config/233322245.json --board config/charuco.json --frames 60 --timeout 60
```

- Streams only color, at the camera's highest resolution in RGB8 or BGR8, at
  the lowest fps for it. That allows the longest exposure, so less noise in dim
  light. From the camera config only the serial is used.
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

`position` is the camera's position in the board frame. `camera_to_world` maps a
point in the color camera frame to the board frame. A reprojection error well
above 1 px usually means the board file does not match the printed board or the
board moved. The result is printed only, not saved.

In Docker, stop the service first, since a camera can be opened only once.
Compose mounts `config/charuco.json` at `/opt/cam_node/charuco.json`:

```sh
docker compose down
docker compose run --rm cam-node calibrate \
  --config /opt/cam_node/config.json --board /opt/cam_node/charuco.json
```
