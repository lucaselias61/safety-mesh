#!/usr/bin/env python3
"""Records RealSense color and depth to .rrd (Rerun), .db3 or .bag.

    pip install pyrealsense2==2.58.* rerun-sdk==0.43.1
    python3 tools/rec_stream.py --out rec/ [--config config/<serial>.json] [--duration 30]
    rerun rec/<serial>_<UTC start>.rrd

- --out without a suffix is a directory.
- .db3/.bag need no rerun-sdk. PyPI wheels write only .db3.
"""

import argparse
import json
import signal
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

import pyrealsense2 as rs

RERUN_SUFFIX = ".rrd"
# SDK formats. A build writes only one.
SDK_SUFFIXES = (".db3", ".bag")
DEFAULT_SUFFIX = RERUN_SUFFIX

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
                        help="output directory, or a file path ending in .rrd, .db3 or .bag")
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
    if out.suffix in (RERUN_SUFFIX, *SDK_SUFFIXES):
        path = out
    else:
        start = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        path = out / f"{serial}_{start}{DEFAULT_SUFFIX}"
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
    if path.suffix in SDK_SUFFIXES:
        config.enable_record_to_file(str(path))
    return config, enabled


def record(pipeline, duration, on_frameset):
    # None for .db3/.bag. The SDK writes those.
    start = time.monotonic()
    frames = 0
    while not stop_requested:
        if duration and time.monotonic() - start >= duration:
            break
        success, frameset = pipeline.try_wait_for_frames(1000)
        if success:
            if on_frameset:
                on_frameset(frames, frameset)
            frames += 1
    return frames, time.monotonic() - start


def rerun_writer(path, profile):
    # Lazy import. .db3/.bag work without rerun-sdk.
    import rerun_log

    rec = rerun_log.open_file(path)
    root = rerun_log.log_profile(rec, profile)
    return rec, lambda index, frameset: rerun_log.log_frameset(rec, root, index, frameset)


def main():
    args = parse_args()
    camera = load_camera_config(args.config)
    serial = select_serial(args.serial, camera.get("serial", ""))
    path = output_path(args.out, serial)
    config, enabled = build_config(serial, camera["streams"], path)

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    pipeline = rs.pipeline()
    profile = pipeline.start(config)
    rec, on_frameset = None, None
    try:
        if path.suffix == RERUN_SUFFIX:
            rec, on_frameset = rerun_writer(path, profile)
        print(f"Recording camera {serial} to {path}", flush=True)
        for line in enabled:
            print(f"  {line}", flush=True)
        frames, elapsed = record(pipeline, args.duration, on_frameset)
    finally:
        # stop() closes .db3/.bag.
        pipeline.stop()
        if rec:
            rec.flush()

    size_mb = path.stat().st_size / 1e6
    print(f"Saved {frames} framesets in {elapsed:.1f} s, {size_mb:.1f} MB: {path}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError) as error:
        print(f"rec_stream failed: {error}", file=sys.stderr, flush=True)
        sys.exit(1)
