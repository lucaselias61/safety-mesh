"""Startup behavior without requiring Docker, CUDA, or a connected camera."""

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "entrypoint", Path(__file__).resolve().parents[1] / "docker/entrypoint.py"
)
entrypoint = importlib.util.module_from_spec(spec)
spec.loader.exec_module(entrypoint)


class StartupTests(unittest.TestCase):
    def test_single_camera_is_selected(self):
        config = {"camera": {"serial": ""}}
        with patch.dict(os.environ, {"CAMERA_SERIAL": ""}), patch.object(
            entrypoint, "capture", return_value="123\tRealSense D455"
        ):
            entrypoint.select_camera(config)
        self.assertEqual(config["camera"]["serial"], "123")

    def test_ambiguous_or_missing_camera_is_rejected(self):
        for devices in ("No RealSense device connected", "123\tD455\n456\tD435"):
            with self.subTest(devices=devices), patch.dict(os.environ, {"CAMERA_SERIAL": ""}), patch.object(
                entrypoint, "capture", return_value=devices
            ), self.assertRaises(ValueError):
                entrypoint.select_camera({"camera": {"serial": ""}})

    def test_explicit_serial_overrides_config_without_discovery(self):
        config = {"camera": {"serial": "123"}}
        with patch.dict(os.environ, {"CAMERA_SERIAL": "456"}), patch.object(entrypoint, "capture") as capture:
            entrypoint.select_camera(config)
        self.assertEqual(config["camera"]["serial"], "456")
        capture.assert_not_called()

    def test_cpu_backend_skips_gpu_setup(self):
        with patch.object(entrypoint, "capture") as capture:
            entrypoint.prepare_engine({"engine": {"backend": "opencv_dnn"}})
        capture.assert_not_called()

    def test_engine_cache_reuse_and_invalidation(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "model.onnx"
            model.write_bytes(b"model-v1")
            versions = {"gpu": "GPU-1, A2000, 595", "trt": "10.16.1"}

            def capture(*args):
                return versions["gpu"] if args[0] == "nvidia-smi" else versions["trt"]

            def build(args, **kwargs):
                self.assertIn("--skipInference", args)
                self.assertIn("--shapes=images:1x3x640x640", args)
                self.assertNotIn("--fp16", args)
                output = next(arg.split("=", 1)[1] for arg in args if arg.startswith("--saveEngine="))
                Path(output).write_bytes(b"complete-engine")

            def prepare():
                config = {"engine": {"backend": "tensorrt", "model_path": str(model), "input_size": 640}}
                entrypoint.prepare_engine(config)
                return Path(config["engine"]["model_path"])

            with patch.object(
                entrypoint, "capture", side_effect=capture
            ), patch.object(entrypoint.shutil, "which", return_value="trtexec"), patch.object(
                Path, "read_text", return_value='{"cuda":"13.2"}'
            ), patch.object(entrypoint.subprocess, "run", side_effect=build) as run:
                first = prepare()
                self.assertEqual(first, prepare())
                self.assertEqual(run.call_count, 1)
                model.write_bytes(b"model-v2")
                second = prepare()
                self.assertNotEqual(first, second)
                versions["gpu"] = "GPU-2, A2000, 595"
                third = prepare()
                self.assertNotEqual(second, third)
                versions["trt"] = "10.16.2"
                fourth = prepare()
                self.assertNotEqual(third, fourth)
                # A failed/incomplete previous publication must be rebuilt.
                fourth.with_suffix(".json").unlink()
                self.assertEqual(fourth, prepare())
                self.assertEqual(run.call_count, 5)
            with fourth.with_suffix(".json").open() as metadata:
                self.assertEqual(json.load(metadata)["tensorrt"], "10.16.2")

    def test_failed_engine_build_publishes_no_plan(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "model.onnx"
            model.write_bytes(b"model")
            config = {"engine": {"backend": "tensorrt", "model_path": str(model), "input_size": 640}}
            with patch.object(entrypoint, "capture", return_value="version"), patch.object(
                entrypoint.shutil, "which", return_value="trtexec"
            ), patch.object(Path, "read_text", return_value="cuda-version"), patch.object(
                entrypoint.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "trtexec")
            ), self.assertRaises(subprocess.CalledProcessError):
                entrypoint.prepare_engine(config)
            self.assertEqual(list((model.parent / ".engines").glob("*.engine")), [])
            self.assertEqual(list((model.parent / ".engines").glob("*.json")), [])

    def test_diagnostic_command_bypasses_startup(self):
        with patch.object(entrypoint.sys, "argv", ["entrypoint.py", "list"]), patch.object(
            entrypoint.os, "execv", side_effect=SystemExit(0)
        ) as execute, patch.object(entrypoint, "prepare_engine") as prepare:
            with self.assertRaises(SystemExit):
                entrypoint.main()
        execute.assert_called_once_with(entrypoint.CAM_NODE, [entrypoint.CAM_NODE, "list"])
        prepare.assert_not_called()


if __name__ == "__main__":
    unittest.main()
