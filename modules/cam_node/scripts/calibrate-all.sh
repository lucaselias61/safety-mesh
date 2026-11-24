#!/usr/bin/env bash

# Calibrates every connected camera, one after another:
# - Finds them with cam_node list, in the calibrate image.
# - Runs compose.calibrate.yaml for each. Writes ${CALIBRATION_DIR}/<serial>.json.
# - Stop the cam-node containers first. A camera opens only once.
# - Keep the board still until all cameras are done, so they share one world frame.
# - Arguments go to calibrate, e.g. --frames 60 --timeout 60.
# - Exits non-zero if any camera failed. Its older pose file stays as it was.

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

# The serial only names the project here. list needs none.
serials="$(CAMERA_SERIAL=list docker compose -f compose.calibrate.yaml run --rm \
    --entrypoint /opt/cam_node/bin/cam_node calibrate list | cut -f1 | grep -E '^[0-9]+$' || true)"

failed=()
found=0
for serial in $serials; do
    found=1
    echo "Calibrating $serial"
    CAMERA_SERIAL="$serial" docker compose -f compose.calibrate.yaml run --rm calibrate "$@" ||
        failed+=("$serial")
done

if [[ "$found" -eq 0 ]]; then
    echo "No camera connected." >&2
    exit 1
fi
if [[ "${#failed[@]}" -gt 0 ]]; then
    echo "Failed: ${failed[*]}" >&2
    exit 1
fi
echo "All cameras calibrated."
