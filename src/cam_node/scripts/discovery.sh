#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
discovery_binary="$project_dir/build/discovery"

if [[ ! -x "$discovery_binary" ]]; then
    echo "Discovery executable not found. Run $project_dir/scripts/build.sh first." >&2
    exit 1
fi

exec "$discovery_binary" "$@"
