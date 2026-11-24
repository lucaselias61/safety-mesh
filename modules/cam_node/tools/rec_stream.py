#!/usr/bin/env python3
"""Records RealSense color and depth to .rrd (Rerun), .db3 or .bag.

    pip install pyrealsense2==2.58.* rerun-sdk==0.43.1
    python3 tools/rec_stream.py --out rec/ [--env .env] [--duration 30]
    rerun rec/<serial>_<UTC start>.rrd

- --out without a suffix is a directory.
- Streams and serial from cam_node's .env, like cam_node run. Defaults without one.
- .db3/.bag need no rerun-sdk. PyPI wheels write only .db3.
"""

import argparse
import signal
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

import pyrealsense2 as rs

# Only .rrd needs rerun-sdk.
try:
    import numpy as np
    import rerun as rr
except ImportError:
    rr = None

RERUN_SUFFIX = ".rrd"
# SDK formats. A build writes only one.
SDK_SUFFIXES = (".db3", ".bag")
DEFAULT_SUFFIX = RERUN_SUFFIX

DEFAULT_ENV = Path(__file__).resolve().parent.parent / ".env"
# Same as .env.example.
DEFAULT_SETTINGS = {
    "COLOR_ENABLED": "true", "COLOR_FORMAT": "RGB8",
    "DEPTH_ENABLED": "true", "DEPTH_FORMAT": "Z16",
    "WIDTH": "640", "HEIGHT": "480", "FPS": "15",
}
STREAM_TYPES = {"color": rs.stream.color, "depth": rs.stream.depth}

# Same as APP_ID in rerun/tools/blueprint.py, so its layout applies.
RERUN_APP_ID = "safety_mesh"
# Same as depth_color_range_m in include/viewer_sink.hpp.
DEPTH_COLOR_RANGE_M = 6.0
# Timestamps in ms since the epoch.
EPOCH_DOMAINS = (rs.timestamp_domain.global_time, rs.timestamp_domain.system_time)
COLOR_MODELS = {rs.format.rgb8: "RGB", rs.format.bgr8: "BGR", rs.format.rgba8: "RGBA", rs.format.bgra8: "BGRA"}

stop_requested = False


def request_stop(_signum, _frame):
    global stop_requested
    stop_requested = True


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", required=True, type=Path,
                        help="output directory, or a file path ending in .rrd, .db3 or .bag")
    parser.add_argument("--serial", help="camera serial")
    parser.add_argument("--env", type=Path, default=DEFAULT_ENV,
                        help="cam_node .env with the stream settings (default modules/cam_node/.env)")
    parser.add_argument("--duration", type=float, default=0,
                        help="seconds to record, 0 = until Ctrl+C (default)")
    return parser.parse_args()


def read_env(path):
    # KEY=VALUE lines. Comments, blanks and quotes are skipped.
    values = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip().strip("'\"")
    return values


def load_camera_config(path):
    # Same settings as cam_node run. Empty values keep the defaults.
    found = read_env(path) if path.exists() else {}
    env = {**DEFAULT_SETTINGS, **{key: value for key, value in found.items() if value}}
    streams = {}
    for name in STREAM_TYPES:
        prefix = name.upper()
        streams[name] = {
            "enabled": env[f"{prefix}_ENABLED"].lower() in ("true", "1", "yes", "on"),
            "format": env[f"{prefix}_FORMAT"],
            "width": int(env["WIDTH"]),
            "height": int(env["HEIGHT"]),
            "fps": int(env["FPS"]),
        }
    return {"serial": env.get("CAMERA_SERIAL", ""), "streams": streams}


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
        raise ValueError("No color or depth stream enabled. Set COLOR_ENABLED or DEPTH_ENABLED.")
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


def video_profile(profile, stream_type):
    for stream in profile.get_streams():
        if stream.stream_type() == stream_type:
            return stream.as_video_stream_profile()
    return None


def pinhole(intrinsics):
    return rr.Pinhole(
        focal_length=[intrinsics.fx, intrinsics.fy],
        principal_point=[intrinsics.ppx, intrinsics.ppy],
        width=intrinsics.width,
        height=intrinsics.height,
        camera_xyz=rr.ViewCoordinates.RDF,
    )


def transform(extrinsics):
    # rs2 rotation is column-major.
    r = extrinsics.rotation
    rows = [[r[col * 3 + row] for col in range(3)] for row in range(3)]
    return rr.Transform3D(translation=extrinsics.translation, mat3x3=rows)


def log_calibration(rec, root, profile):
    # Same as ViewerSink::calibration. Color is the root frame.
    color = video_profile(profile, rs.stream.color)
    depth = video_profile(profile, rs.stream.depth)
    if color:
        rec.log(f"{root}/color", pinhole(color.get_intrinsics()), static=True)
    if depth:
        archetypes = [pinhole(depth.get_intrinsics())]
        if color:
            archetypes.insert(0, transform(depth.get_extrinsics_to(color)))
        rec.log(f"{root}/depth", *archetypes, static=True)


def log_frameset(rec, root, index, frameset):
    color = frameset.get_color_frame()
    depth = frameset.get_depth_frame()
    reference = color or depth
    if not reference:
        return
    rec.set_time("frame", sequence=index)
    # Hardware clock is not epoch based. Only the frame timeline then.
    if reference.get_frame_timestamp_domain() in EPOCH_DOMAINS:
        rec.set_time("capture_time", timestamp=reference.get_timestamp() / 1000)
    model = color and COLOR_MODELS.get(color.get_profile().format())
    if model:
        rec.log(f"{root}/color", rr.Image(np.asanyarray(color.get_data()), color_model=model))
    if depth and depth.get_profile().format() == rs.format.z16:
        units_per_meter = 1.0 / depth.get_units()
        rec.log(f"{root}/depth", rr.DepthImage(
            np.asanyarray(depth.get_data()),
            meter=units_per_meter,
            depth_range=(0.0, DEPTH_COLOR_RANGE_M * units_per_meter),
        ))


def rerun_writer(path, serial, profile):
    # Logged like cam_node (src/viewer_sink.cpp). Own recording id, so no mix with live data.
    rec = rr.RecordingStream(RERUN_APP_ID, recording_id=f"rec_stream/{path.name}")
    rec.save(str(path))
    root = f"cams/{serial}"
    log_calibration(rec, root, profile)
    return rec, lambda index, frameset: log_frameset(rec, root, index, frameset)


def main():
    args = parse_args()
    camera = load_camera_config(args.env)
    serial = select_serial(args.serial, camera.get("serial", ""))
    path = output_path(args.out, serial)
    if path.suffix == RERUN_SUFFIX and rr is None:
        raise ValueError("pip install rerun-sdk for .rrd, or use --out with .db3.")
    config, enabled = build_config(serial, camera["streams"], path)

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    pipeline = rs.pipeline()
    profile = pipeline.start(config)
    rec, on_frameset = None, None
    try:
        if path.suffix == RERUN_SUFFIX:
            rec, on_frameset = rerun_writer(path, serial, profile)
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
