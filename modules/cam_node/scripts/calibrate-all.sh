#!/usr/bin/env bash

# Calibrates every camera with a config/<serial>.json, one after another:
# - The configs only list the cameras of this host. calibrate needs just the serial.
# - Runs compose.calibrate.yaml for each. Writes ${CALIBRATION_DIR}/<serial>.json.
# - Stop the cam-node containers first. A camera opens only once.
# - Keep the board still until all cameras are done, so they share one world frame.
# - Arguments go to calibrate, e.g. --frames 60 --timeout 60.
# - Exits non-zero if any camera failed. Its older pose file stays as it was.

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

failed=()
found=0
for config in config/*.json; do
    serial="$(basename "$config" .json)"
    # Skips charuco.json and the example config.
    [[ "$serial" =~ ^[0-9]+$ ]] || continue
    found=1
    echo "Calibrating $serial"
    CAMERA_SERIAL="$serial" docker compose -f compose.calibrate.yaml run --rm calibrate "$@" ||
        failed+=("$serial")
done

if [[ "$found" -eq 0 ]]; then
    echo "No config/<serial>.json found." >&2
    exit 1
fi
if [[ "${#failed[@]}" -gt 0 ]]; then
    echo "Failed: ${failed[*]}" >&2
    exit 1
fi
echo "All cameras calibrated."
