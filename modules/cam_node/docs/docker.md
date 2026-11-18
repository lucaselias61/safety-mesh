# Docker deployment

Run these commands from `modules/cam_node/` on a native Linux PC. The default
TensorRT setup needs x86-64 and an NVIDIA GPU. The CPU-only setup (`INFERENCE=cpu`)
builds for the host's own architecture, including arm64.
The image contains CUDA, TensorRT, OpenCV, RealSense, and the compiled camera node.
It does not run the native installation scripts.

## Host setup (once per PC)

Install a working NVIDIA driver compatible with CUDA 13.2 using
[Ubuntu's driver installation guide](https://ubuntu.com/server/docs/nvidia-drivers-installation/),
and verify `nvidia-smi` on the host. Then install Docker Engine, the Compose plugin and the
NVIDIA Container Toolkit:

```sh
./scripts/install-docker-host.sh
```

The script requires Ubuntu 24.04 or newer on x86-64. It follows
[Docker's](https://docs.docker.com/engine/install/ubuntu/) and
[NVIDIA's](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)
APT instructions with pinned versions, configures Docker to use the NVIDIA runtime, and
finishes by running `nvidia-smi` in a container. It stops if conflicting packages such
as `docker.io` are installed, and does not install the driver.

If the runtime configuration changes, the script restarts Docker, which interrupts
running containers. Rootless Docker and Docker Desktop
camera passthrough are not covered by this configuration. Use `sudo docker compose`
if your account cannot access Docker.

The image is built on `ubuntu:24.04` from one multi-stage `Dockerfile`. Each compose
file picks its final stage with `build.target`: `cpu` or `gpu`. BuildKit builds only
the stages that target needs. The final image holds only runtime libraries: OpenCV,
RealSense and the compiled camera node, plus TensorRT 10.16.1 and the CUDA 13.2
runtime from NVIDIA's APT repository in `gpu`. Compilers and headers stay in the
build stages. `INFERENCE=cpu` never touches NVIDIA's repository. The versions are the
`CUDA_VERSION` and `TENSORRT_VERSION` arguments at the top of the `Dockerfile`.
Host CUDA/TensorRT packages are not required. The host NVIDIA driver must support
CUDA 13.2.

## Start

The Compose setup is split in two files:

- `compose.cpu.yaml`: OpenCV DNN on the CPU. Holds the application, USB access, model
  and configuration settings. Builds for the host's own architecture.
- `compose.gpu.yaml`: TensorRT on an NVIDIA GPU. Extends the `cam-node` service of
  `compose.cpu.yaml` and adds the TensorRT build, the GPU reservation and the
  `linux/amd64` platform.

`.env` picks one through `INFERENCE`. Its `COMPOSE_FILE` line loads
`compose.${INFERENCE}.yaml`, so plain `docker compose` commands use the right file.
Create `.env` once:

```sh
cp .env.example .env
```

Set `INFERENCE=gpu` or `INFERENCE=cpu` there, in lowercase. There is no
`compose.yaml`, so without `.env` plain `docker compose` finds no file. Use
`docker compose -f compose.gpu.yaml` or `-f compose.cpu.yaml` then. A `-f` option or an
exported `COMPOSE_FILE` replaces the `.env` selection.

In each new terminal, set your camera serial and create its configuration:

```sh
export CAMERA_SERIAL=123456789
cp config/serial_number.example.json "config/${CAMERA_SERIAL}.json"
```

Edit `config/123456789.json`: set `camera.serial` to `123456789`,
`engine.backend` to `tensorrt` (keep `opencv_dnn` with `INFERENCE=cpu`), and
`engine.model_path` to `/models/yolo11n.onnx`. The example uses native execution
defaults, so these engine settings must be changed for Docker. Compose uses the serial to name both the project and container
`cam-123456789`. Connect the selected RealSense camera to USB 3, then:

```sh
docker compose up --build -d
docker compose logs -f cam-node
```

The first image build downloads dependencies and compiles RealSense and `cam_node`;
it needs internet access but no GPU. Compose also passes `../rerun` (the shared
Rerun connection library) as an extra build context, which needs Docker Compose 2.17
or newer; build through Compose rather than with a plain `docker build`. The ONNX model is already in `models/`.
Startup selects the configured camera and starts `cam_node`. With `tensorrt` it builds
an engine on the selected GPU before detection starts. Engine compilation can take several
minutes. Subsequent starts reuse the cached engine.

`CAM_NODE_CONFIG_PATH` (default `config/${CAMERA_SERIAL}.json`) is the host
config file, absolute or relative to this directory. It is mounted read-only at
`/opt/cam_node/config.json` and must exist before starting it.
`CAMERA_SERIAL` overrides its camera serial. Each camera uses its own configuration
file. If you do not know the serial, build the image and list cameras using the
example config:

```sh
CAMERA_SERIAL=discovery docker compose build
CAMERA_SERIAL=discovery CAM_NODE_CONFIG_PATH=config/serial_number.example.json \
  docker compose run --rm cam-node list
```

Then set the discovered serial and create its configuration as shown above.

Compose accepts `CAMERA_SERIAL`, `GPU_DEVICE` (host GPU index or
UUID, default `0`, `INFERENCE=gpu` only), `BUILD_JOBS` (default `2`), `INFERENCE` (`gpu` or
`cpu`, default `gpu` in `.env.example`, selects the compose file as described above),
`RERUN` (`ON` or `OFF`, default `ON`; `OFF` leaves the Rerun SDK out of the build),
`LOG_LEVEL` (default `info`, see
[logging](configuration.md#logging)), and the optional
[Rerun settings](configuration.md#rerun-live-debugging) `RERUN_URL` and
`RERUN_IMAGE_FPS` from the shell or the untracked `.env` file beside the compose
files. With the viewer from `modules/rerun` on the same host, use
`RERUN_URL=rerun+http://host.docker.internal:9876/proxy`.

Each build job can use 1–2 GB of RAM, mostly while compiling librealsense and
Arrow (used by the Rerun SDK). If the host freezes during the build, lower
`BUILD_JOBS`; use `1` on hosts with 8 GB or less. The CMake build directory is
kept in a BuildKit cache between builds, so Arrow and Rerun compile once and
later builds only recompile changed `cam_node` files. CPU and GPU builds keep
separate caches, so switching `INFERENCE` does not recompile the other. Changing
`BUILD_JOBS` rebuilds librealsense. Clear the cache with `docker builder prune` if a build
behaves oddly.

You can also supply the serial for an individual command:

```sh
CAMERA_SERIAL=123456789 docker compose up --build -d
```

Relative paths resolve from the compose files' directory. All commands below
assume `CAMERA_SERIAL` is exported as shown above. If you previously exported
`COMPOSE_FILE`, run `unset COMPOSE_FILE` first so the `.env` selection applies.

Edit camera stream settings to match the connected camera's supported profiles.
To inspect them, use the existing CLI through Compose, for example
`docker compose run --rm cam-node --help`. Diagnostic commands bypass camera selection and do not build engines.

## Model and engine storage

The host `models/` directory is mounted at `/models` and must be writable by the
container. With `tensorrt`, the Docker configuration's `engine.model_path` names the
**ONNX source**, and `cam_node` builds and caches the engine as in native execution
([GPU acceleration](inference.md#gpu-acceleration)).
Use the [documented YOLO model layout](inference.md#model-requirements).

Engines and metadata persist under `models/.engines/` across container replacement.
The cache key includes model contents, input size, GPU identity, CUDA driver and
runtime, and TensorRT versions. Changing these generates a new engine. Builds use
a lock and temporary files so failed or concurrent builds do not publish partial plans.
Old cache entries are retained; stop the service before deleting unused entries.
Entries created by earlier images that built engines with `trtexec` are not reused.
Files created by the default container user may be owned by root on the host.

## Camera access and permissions

With `INFERENCE=gpu`, Compose grants GPU access using an NVIDIA device reservation.
Both setups mount `/dev/bus/usb` and allow USB device major `189`. Neither uses
privileged mode. RealSense is
built with `FORCE_RSUSB_BACKEND=ON`, so it uses userspace USB rather than requiring
RealSense-specific kernel patches. Validate capture stability and frame rates on
your camera and host.

If USB permissions prevent access, install the rules shipped with the pinned SDK
onto the host (this requires a successful image build):

```sh
docker compose build
docker compose run --rm --no-deps --entrypoint cat cam-node \
  /opt/cam_node/99-realsense-libusb.rules > /tmp/cam-node-realsense.rules
sudo install -m 0644 /tmp/cam-node-realsense.rules \
  /etc/udev/rules.d/99-realsense-libusb.rules
sudo udevadm control --reload-rules
```

Unplug and reconnect the camera, then restart the service. Host udev rules cannot
be installed by an ordinary Dockerfile build.

## Stop and verify

```sh
docker compose exec cam-node nvidia-smi   # INFERENCE=gpu only
docker compose logs --tail=100 cam-node
docker compose down
```

`unless-stopped` restarts the service after an exit or host reboot when Docker is
enabled at boot. A missing camera or invalid configuration produces a startup error;
inspect logs if the service keeps restarting. Docker keeps the newest five 10 MB
log files per container and discards older output. SIGTERM reaches the application through
the startup wrapper and Compose's init process.

Validate image compilation, camera enumeration, first engine generation, cached
restart, detection, and shutdown on the target PC before deployment. Python startup
tests can run without Docker or a GPU:

```sh
python3 -m unittest discover -s tests/unit -p 'test_docker_entrypoint.py'
```

## C++ tests

Tests live in `tests/unit/` (no camera, no GPU) and `tests/live/` (need the
configured camera). Live tests carry the ctest label `live` and are only built
with `-DCAM_NODE_LIVE_TESTS=ON`.

### Unit tests

The unit tests run in the `Dockerfile`'s `test` stage, on the same Ubuntu
24.04 toolchain and OpenCV 4.6 as the images. They work on any host with
Docker, including macOS:

```sh
./scripts/test.sh
```

- The stage builds on `build-base` without Rerun, so Arrow is not compiled. It
  builds every target, live tests included, so everything gets a compile check.
  It runs `ctest -LE live`.
- A failing test fails the build. The stage is never saved as an image, and
  `docker compose build` skips it.
- Changed files recompile incrementally from a BuildKit cache.
- With unchanged sources the test step shows `CACHED`, meaning it passed before.
  Run `./scripts/test.sh --no-cache-filter test` to run it again.
- `BUILD_JOBS` comes from the shell or `.env`, as for Compose, so the shared
  build layers stay cached.

### Tests on the target machine

`compose.test.yaml` runs both kinds in one `test-runner` image, built from the
`Dockerfile` stage of that name. Each service picks its tests with a ctest label.
Neither needs `CAMERA_SERIAL`:

```sh
docker compose -f compose.test.yaml run --rm test-cam-node-unit   # ctest -LE live
docker compose -f compose.test.yaml run --rm test-cam-node-live   # ctest -L live
```

`scripts/test.sh` runs the same unit tests during the build and leaves no image
behind, so it suits development and CI.

### Live tests

`cam_pipeline_test` checks every connected camera, one after another. Each camera
uses its own `config/<serial>.json`. A camera without one is checked with the
streams of `config/serial_number.example.json`, and the output names the file
used. A camera disabled in its config is skipped. The service gets the same USB
access as `cam-node` and the `config/` directory read-only. Stop the `cam-node`
service first, since a camera can be opened only once:

```sh
docker compose down
docker compose -f compose.test.yaml run --rm test-cam-node-live
```

The test runs cam_node's own config loading and `Pipeline`, and reports each
step as PASS or FAIL with the camera's serial:

- at least one camera is connected
- the config loads, the camera is ready and on USB 3
- the configured streams are offered by the camera
- every streaming sensor supports global time, and the pipeline starts in it
- depth and color calibration is available, if both are configured
- over 5 s, every stream reaches 90% of its fps, drops at most 5% of frames,
  has increasing timestamps, stays in global time, and has a frame age under
  three frame periods
- the color image is not flat and at least 5% of depth pixels have a distance
- the pipeline stops

Point the cameras at a normal scene. A covered lens or an empty view beyond depth
range fails the content checks.

Natively on Ubuntu, build with `-DCAM_NODE_LIVE_TESTS=ON` and pass the config
directory:

```sh
CAM_NODE_CONFIG_DIR="$PWD/config" ctest --test-dir build/cpu -L live --output-on-failure
```
