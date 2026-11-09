"""Prepare a camera config, then exec cam_node."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


CAM_NODE = "/opt/cam_node/bin/cam_node"


def capture(*args):
    return subprocess.check_output(args, text=True).strip()


def select_camera(config):
    serial = os.environ.get("CAMERA_SERIAL") or config["camera"]["serial"]
    if not serial:
        lines = capture(CAM_NODE, "list").splitlines()
        serials = [line.split("\t", 1)[0] for line in lines if "\t" in line]
        if len(serials) != 1:
            raise ValueError(
                f"Found {len(serials)} cameras. Connect one camera or set CAMERA_SERIAL."
            )
        serial = serials[0]
    config["camera"]["serial"] = serial


def main():
    # Diagnostic commands bypass automatic camera selection.
    if len(sys.argv) > 1:
        os.execv(CAM_NODE, [CAM_NODE, *sys.argv[1:]])
    with Path("/config/camera.json").open() as source:
        config = json.load(source)
    if not config["camera"]["enabled"]:
        print("Camera disabled; exiting.", flush=True)
        return
    select_camera(config)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as runtime:
        json.dump(config, runtime)
        config_path = runtime.name
    print(f"Starting cam_node with camera {config['camera']['serial']}", flush=True)
    # Flush C++ detection logs promptly when Docker captures stdout.
    os.execvp("stdbuf", ["stdbuf", "-oL", "-eL", CAM_NODE, "run", "--config", config_path])


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"cam_node startup failed: {error}", file=sys.stderr, flush=True)
        sys.exit(1)
