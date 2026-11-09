"""Startup behavior without requiring Docker, CUDA, or a connected camera."""

import importlib.util
import os
from pathlib import Path
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

    def test_diagnostic_command_bypasses_startup(self):
        with patch.object(entrypoint.sys, "argv", ["entrypoint.py", "list"]), patch.object(
            entrypoint.os, "execv", side_effect=SystemExit(0)
        ) as execute, patch.object(entrypoint, "select_camera") as select:
            with self.assertRaises(SystemExit):
                entrypoint.main()
        execute.assert_called_once_with(entrypoint.CAM_NODE, [entrypoint.CAM_NODE, "list"])
        select.assert_not_called()


if __name__ == "__main__":
    unittest.main()
