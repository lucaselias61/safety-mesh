#!/usr/bin/env python3
"""Records the depth and color streams of a RealSense camera to a .bag file.

    # match REALSENSE_VERSION
    pip install pyrealsense2==2.58.*   
    python3 tools/rec_stream.py --out recordings/
    # optional config file
    python3 tools/rec_stream.py --out recordings/ --config config/233322245.json --duration 30
    python3 tools/rec_stream.py --out take1.bag --serial 233322245

- --out ending in .bag is the file path.
- Without --serial the only connected camera is used.
- Stops after --duration seconds, or on Ctrl+C / SIGTERM.
- The bag keeps raw Z16 depth, color, timestamps, intrinsics and depth scale.
  Open it in realsense-viewer or replay it with rs2::config::enable_device_from_file.
- Uncompressed. 640x480 Z16 + RGB8 at 15 fps is about 23 MB/s.
"""

import argparse
import json
import signal
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

import pyrealsense2 as rs

# Same as config/serial_number.example.json.
DEFAULT_STREAMS = {
    "color": {"enabled": True, "format": "RGB8", "width": 640, "height": 480, "fps": 15},
    "depth": {"enabled": True, "format": "Z16", "width": 640, "height": 480, "fps": 15},
}
STREAM_TYPES = {"color": rs.stream.color, "depth": rs.stream.depth}

stop_requested = False


def request_stop(_signum, _frame):
    global stop_requested
    stop_requested = True


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", required=True, type=Path,
                        help="output directory, or a file path ending in .bag")
    parser.add_argument("--serial", help="camera serial")
    parser.add_argument("--config", type=Path, help="cam_node camera config (config/<serial>.json)")
    parser.add_argument("--duration", type=float, default=0,
                        help="seconds to record, 0 = until Ctrl+C (default)")
    return parser.parse_args()


def load_camera_config(path):
    if path is None:
        return {"serial": "", "streams": DEFAULT_STREAMS}
    with path.open() as source:
        return json.load(source)["camera"]


def connected_serials():
    return [device.get_info(rs.camera_info.serial_number) for device in rs.context().query_devices()]


def select_serial(requested, config_serial):
    serials = connected_serials()
    serial = requested or config_serial
    if serial:
        if serial not in serials:
            raise ValueError(f"Camera {serial} not found. Connected: {serials or 'none'}")
        return serial
    if len(serials) != 1:
        raise ValueError(f"Found {len(serials)} cameras. Connect one camera or pass --serial.")
    return serials[0]


def output_path(out, serial):
    # .bag suffix means a file path. Anything else is a directory.
    if out.suffix == ".bag":
        path = out
    else:
        start = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        path = out / f"{serial}_{start}.bag"
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        raise ValueError(f"{path} exists. Not overwriting it.")
    return path


def build_config(serial, streams, path):
    config = rs.config()
    config.enable_device(serial)
    enabled = []
    for name, stream_type in STREAM_TYPES.items():
        stream = streams.get(name, {})
        if not stream.get("enabled", False):
            continue
        stream_format = getattr(rs.format, stream["format"].lower())
        config.enable_stream(stream_type, stream["width"], stream["height"], stream_format, stream["fps"])
        enabled.append(f"{name} {stream['width']}x{stream['height']} {stream['format']} @ {stream['fps']} fps")
    if not enabled:
        raise ValueError("No color or depth stream enabled in the config.")
    config.enable_record_to_file(str(path))
    return config, enabled


def record(pipeline, duration):
    # The SDK writes every frame to the bag. This loop only keeps the pipeline running.
    start = time.monotonic()
    frames = 0
    while not stop_requested:
        if duration and time.monotonic() - start >= duration:
            break
        success, _ = pipeline.try_wait_for_frames(1000)
        if success:
            frames += 1
    return frames, time.monotonic() - start


def main():
    args = parse_args()
    camera = load_camera_config(args.config)
    serial = select_serial(args.serial, camera.get("serial", ""))
    path = output_path(args.out, serial)
    config, enabled = build_config(serial, camera["streams"], path)

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    pipeline = rs.pipeline()
    pipeline.start(config)
    print(f"Recording camera {serial} to {path}", flush=True)
    for line in enabled:
        print(f"  {line}", flush=True)
    try:
        frames, elapsed = record(pipeline, args.duration)
    finally:
        # Without stop() the bag is not closed and cannot be read.
        pipeline.stop()

    size_mb = path.stat().st_size / 1e6
    print(f"Saved {frames} framesets in {elapsed:.1f} s, {size_mb:.1f} MB: {path}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError) as error:
        print(f"rec_stream failed: {error}", file=sys.stderr, flush=True)
        sys.exit(1)
