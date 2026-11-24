"""Prepare a camera config, then exec cam_node."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


CAM_NODE = "/opt/cam_node/bin/cam_node"
# Mount target of the host config file (compose.yaml).
CONFIG_PATH = "/opt/cam_node/config.json"


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


def parse_bool(text):
    if text.lower() in ("1", "true", "yes", "on"):
        return True
    if text.lower() in ("0", "false", "no", "off"):
        return False
    raise ValueError(f"Expected true or false, got '{text}'.")


COLOR = ("camera", "streams", "color")
DEPTH = ("camera", "streams", "depth")

# Env var -> config keys and type. Set vars override the mounted config file.
# - WIDTH, HEIGHT and FPS set color and depth, they are always the same.
ENV_KEYS = {
    "CAMERA_ENABLED": ([("camera", "enabled")], parse_bool),
    "COLOR_ENABLED": ([(*COLOR, "enabled")], parse_bool),
    "COLOR_FORMAT": ([(*COLOR, "format")], str),
    "DEPTH_ENABLED": ([(*DEPTH, "enabled")], parse_bool),
    "DEPTH_FORMAT": ([(*DEPTH, "format")], str),
    "WIDTH": ([(*COLOR, "width"), (*DEPTH, "width")], int),
    "HEIGHT": ([(*COLOR, "height"), (*DEPTH, "height")], int),
    "FPS": ([(*COLOR, "fps"), (*DEPTH, "fps")], int),
    "ENGINE_BACKEND": ([("engine", "backend")], str),
    "MODEL_PATH": ([("engine", "model_path")], str),
    "CONFIDENCE_THRESHOLD": ([("engine", "confidence_threshold")], float),
    "NMS_THRESHOLD": ([("engine", "nms_threshold")], float),
    "INPUT_SIZE": ([("engine", "input_size")], int),
    "PRECISION": ([("engine", "precision")], str),
}


def apply_env(config):
    for name, (paths, parse) in ENV_KEYS.items():
        text = os.environ.get(name, "")
        if not text:
            continue
        for keys in paths:
            node = config
            for key in keys[:-1]:
                node = node.setdefault(key, {})
            node[keys[-1]] = parse(text)


def load_config(path):
    # No mounted file: every value comes from the env (root compose.yaml).
    if Path(path).exists():
        with Path(path).open() as source:
            config = json.load(source)
    else:
        missing = [name for name in ENV_KEYS if not os.environ.get(name)]
        if missing:
            raise ValueError(f"No {path}. Mount one or set {', '.join(missing)}.")
        config = {"camera": {"serial": ""}}
    apply_env(config)
    return config


def main():
    # Diagnostic commands bypass automatic camera selection.
    if len(sys.argv) > 1:
        os.execv(CAM_NODE, [CAM_NODE, *sys.argv[1:]])
    config = load_config(CONFIG_PATH)
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
