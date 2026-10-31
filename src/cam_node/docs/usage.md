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

## Preview

```sh
./bin/cam_node view --serial 233322245
```

Displays color and a depth heatmap side by side at 640 × 480, 15 FPS. Requires a display and a camera supporting those profiles. Exit with `q`, Escape, closing the window or Ctrl+C.

## Run detection

Copy [the example configuration](../config/cam_node.example.json) and set your camera serial:

```sh
cp config/cam_node.example.json config/cam_node.json
./bin/cam_node run --config config/cam_node.json
```

`run` requires `--config`; it has no `--serial` option. Stop with Ctrl+C. SIGTERM also requests shutdown.

Missing cameras, invalid configuration and initialization failures return a nonzero exit code. See [outputs](outputs.md) for detection results.
