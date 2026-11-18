#!/usr/bin/env bash

# Build and run the C++ unit tests in the Dockerfile's test stage:
# - Ubuntu 24.04 toolchain and OpenCV 4.6, the same as the images.
# - Works on any host with Docker, including macOS.
# - A failing test fails the build. Nothing is left behind but build cache.
# - Unchanged sources print CACHED for the test step: it passed before.
#   Pass --no-cache-filter test to run it again anyway.
# Extra arguments go to docker build.

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"

command -v docker >/dev/null || { echo "Docker is required." >&2; exit 1; }

# Same BUILD_JOBS as docker compose, so the shared build-base layers stay cached.
# Shell value first, then .env, then the compose default.
jobs="${BUILD_JOBS:-}"
if [[ -z "$jobs" && -f .env ]]; then
    jobs="$(sed -n 's/^BUILD_JOBS=//p' .env | tail -n 1)"
fi
jobs="${jobs:-2}"

# The test stage does not use the rerun context. Passing it keeps the build
# valid if BuildKit resolves the other stages' COPY --from=rerun.
# cacheonly: run the stage without saving it as an image.
docker build --target test \
    --output type=cacheonly \
    --build-context rerun=../rerun \
    --build-arg BUILD_JOBS="$jobs" \
    --progress=plain \
    "$@" .
