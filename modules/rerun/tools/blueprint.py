#!/usr/bin/env python3
"""Builds the safety_mesh viewer layout: one tab per stream, one view per camera.

    pip install rerun-sdk==0.43.1   # match RERUN_VERSION in ../Dockerfile
    python3 tools/blueprint.py 233322245 --url rerun+http://localhost:9876/proxy
    python3 tools/blueprint.py 233322245 --out layout.rbl

--url sends the layout to a running viewer and makes it the default for the
safety_mesh app, so viewers that connect later get it too. --out saves it as
a .rbl file instead, for `rerun layout.rbl`. Re-run when cameras change.

The viewer container runs this on every start for RERUN_CAMERAS
(docker/entrypoint.sh). --wait lets it start before the viewer is up.

Paths must match what cam_node logs (src/viewer_sink.cpp) under cams/<serial>.
"""

import argparse
import socket
import time
from urllib.parse import urlparse

import rerun as rr
import rerun.blueprint as rrb

# Must match the app and recording id in src/viewer_connection.cpp.
APP_ID = "safety_mesh"
RECORDING_ID = "safety_mesh"

LATENCY_PATHS = ["wait_ms", "detect_ms", "deproject_ms", "log_ms", "age_ms"]


def color_tab(serials):
    """Color image with detections, keypoints and skeletons."""
    views = [rrb.Spatial2DView(name=s, origin=f"/cams/{s}/color") for s in serials]
    return rrb.Grid(*views, name="Color")


def depth_tab(serials):
    views = [rrb.Spatial2DView(name=s, origin=f"/cams/{s}/depth") for s in serials]
    return rrb.Grid(*views, name="Depth")


def space_tab(serials):
    """Depth point cloud, 3D keypoints and skeletons, in each color camera's frame."""
    views = [rrb.Spatial3DView(name=s, origin=f"/cams/{s}") for s in serials]
    return rrb.Grid(*views, name="3D")


def world_tab():
    """All cameras and robots, placed by the manager's poses."""
    return rrb.Spatial3DView(name="World", origin="/")


def health_tab(serials):
    """Latency and fps per camera, in separate plots since units differ."""
    cameras = []
    for s in serials:
        origin = f"/cams/{s}/health"
        latency = rrb.TimeSeriesView(
            name=f"{s} latency (ms)",
            origin=origin,
            contents=[f"$origin/{path}" for path in LATENCY_PATHS],
        )
        fps = rrb.TimeSeriesView(
            name=f"{s} fps", origin=origin, contents=["$origin/fps"]
        )
        cameras.append(rrb.Vertical(latency, fps))
    return rrb.Grid(*cameras, name="Health")


def blueprint(serials):
    return rrb.Blueprint(
        rrb.Tabs(
            world_tab(),
            color_tab(serials),
            depth_tab(serials),
            space_tab(serials),
            health_tab(serials),
        ),
        collapse_panels=False,
    )


def wait_for_viewer(url, timeout):
    """Blocks until the viewer at url accepts connections. Exits after timeout seconds."""
    address = urlparse(url)
    deadline = time.monotonic() + timeout
    while True:
        try:
            socket.create_connection((address.hostname, address.port), timeout=1).close()
            return
        except OSError:
            if time.monotonic() >= deadline:
                raise SystemExit(f"no viewer at {url} after {timeout:g} s, layout not sent")
            time.sleep(0.5)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("serials", nargs="+", help="RealSense serial numbers")
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--url", help="viewer SDK endpoint, e.g. rerun+http://localhost:9876/proxy")
    target.add_argument("--out", help="write a .rbl file instead")
    parser.add_argument(
        "--wait", type=float, default=0, metavar="SECONDS",
        help="with --url, wait up to SECONDS for the viewer to start",
    )
    args = parser.parse_args()

    layout = blueprint(args.serials)
    if args.out:
        layout.save(APP_ID, args.out)
        print(f"wrote {args.out}")
        return

    if args.wait > 0:
        wait_for_viewer(args.url, args.wait)
    # Same app and recording as the nodes, so no extra recording shows up.
    rec = rr.RecordingStream(APP_ID, recording_id=RECORDING_ID, send_properties=False)
    rec.connect_grpc(args.url)
    rec.send_blueprint(layout, make_active=True, make_default=True)
    rec.flush()
    print(f"sent layout for {', '.join(args.serials)} to {args.url}")


if __name__ == "__main__":
    main()
