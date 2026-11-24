#!/usr/bin/env python3
"""Saves color frames from a RealSense camera as PNG, 1 per second.

    pip install pyrealsense2==2.58.* "opencv-python>=4.7"
    python3 tools/extract_frames.py --out frames/ [--serial 233322245] [--duration 600]

Stops after --duration seconds, or on Ctrl+C.
"""

import argparse
import time
from datetime import datetime, timezone
from pathlib import Path

import cv2
import numpy as np
import pyrealsense2 as rs


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--serial", help="default: the only connected camera")
    parser.add_argument("--fps", type=float, default=1.0, help="frames saved per second (default 1)")
    parser.add_argument("--duration", type=float, default=0, help="seconds, 0 = until Ctrl+C")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    args = parser.parse_args()

    config = rs.config()
    if args.serial:
        config.enable_device(args.serial)
    # Camera runs at 15 fps. Frames in between are dropped.
    config.enable_stream(rs.stream.color, args.width, args.height, rs.format.bgr8, 15)
    pipeline = rs.pipeline()
    profile = pipeline.start(config)
    serial = profile.get_device().get_info(rs.camera_info.serial_number)
    args.out.mkdir(parents=True, exist_ok=True)

    start = time.monotonic()
    next_save = start
    saved = 0
    try:
        while not args.duration or time.monotonic() - start < args.duration:
            frame = pipeline.wait_for_frames().get_color_frame()
            if not frame or time.monotonic() < next_save:
                continue
            next_save += 1 / args.fps
            stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
            cv2.imwrite(str(args.out / f"{serial}_{stamp}.png"), np.asanyarray(frame.get_data()))
            saved += 1
    except KeyboardInterrupt:
        pass
    finally:
        pipeline.stop()
    print(f"Saved {saved} frames to {args.out}")


if __name__ == "__main__":
    main()
