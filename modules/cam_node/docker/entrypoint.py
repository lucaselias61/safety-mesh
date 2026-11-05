"""Prepare a camera config and a GPU-specific engine, then exec cam_node."""

import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
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


def prepare_engine(config):
    engine = config["engine"]
    if engine["backend"] != "tensorrt":
        return
    model = Path(engine["model_path"])
    if not model.is_file() or model.suffix.lower() != ".onnx":
        raise ValueError("Docker startup requires an existing ONNX model_path for tensorrt.")
    size = engine["input_size"]
    if type(size) is not int or size < 1:
        raise ValueError("engine.input_size must be a positive integer.")
    shape = f"images:1x3x{size}x{size}"
    trtexec = shutil.which("trtexec")
    if not trtexec:
        for candidate in ("/opt/tensorrt/bin/trtexec", "/usr/src/tensorrt/bin/trtexec"):
            if os.access(candidate, os.X_OK):
                trtexec = candidate
                break
    if not trtexec:
        raise ValueError("trtexec is missing from the container.")
    fingerprint = {
        "onnx_sha256": hashlib.sha256(model.read_bytes()).hexdigest(),
        "gpu": capture("nvidia-smi", "--query-gpu=uuid,name,driver_version", "--format=csv,noheader"),
        "tensorrt": capture(sys.executable, "-c", "import tensorrt; print(tensorrt.__version__)"),
        "cuda": Path("/usr/local/cuda/version.json").read_text(),
        "container": os.environ.get("TENSORRT_CONTAINER_TAG", "unknown"),
        "shape": shape,
        "io": "fp32-linear",
    }
    key = hashlib.sha256(json.dumps(fingerprint, sort_keys=True).encode()).hexdigest()
    cache = model.parent / ".engines"
    cache.mkdir(parents=True, exist_ok=True)
    plan = cache / f"{key}.engine"
    metadata = plan.with_suffix(".json")
    # Serialize builds sharing the same bind mount. Publish only complete plans.
    with (cache / f"{key}.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if not plan.is_file() or plan.stat().st_size == 0 or not metadata.is_file():
            print(f"Building TensorRT engine on the deployment GPU: {plan}", flush=True)
            with tempfile.TemporaryDirectory(dir=cache) as staging:
                temporary_plan = Path(staging) / "model.engine"
                subprocess.run([
                    trtexec, f"--onnx={model}", f"--saveEngine={temporary_plan}",
                    f"--shapes={shape}", "--skipInference",
                ], check=True)
                if not temporary_plan.is_file() or temporary_plan.stat().st_size == 0:
                    raise ValueError("trtexec did not produce a nonempty engine.")
                temporary_metadata = Path(staging) / "metadata.json"
                temporary_metadata.write_text(json.dumps(fingerprint, indent=2) + "\n")
                temporary_plan.replace(plan)
                temporary_metadata.replace(metadata)
        else:
            print(f"Using cached TensorRT engine: {plan}", flush=True)
    engine["model_path"] = str(plan)


def main():
    # Diagnostic commands bypass model generation and automatic camera selection.
    if len(sys.argv) > 1:
        os.execv(CAM_NODE, [CAM_NODE, *sys.argv[1:]])
    with Path("/config/camera.json").open() as source:
        config = json.load(source)
    if not config["camera"]["enabled"]:
        print("Camera disabled; exiting.", flush=True)
        return
    select_camera(config)
    prepare_engine(config)
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
