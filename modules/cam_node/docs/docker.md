# Docker deployment

Run these commands from `modules/cam_node/` on a native x86-64 Linux PC.
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

The image uses NVIDIA's `tensorrt:26.05-py3` (TensorRT 10, Ubuntu 24.04 userspace)
on the Linux host, including Ubuntu 26.04. Host CUDA/TensorRT packages are not required.
The host NVIDIA driver must still satisfy the container's requirements.

## Start

`compose.yaml` contains the application, GPU, USB access, model, and configuration
settings. In each new terminal, set your camera serial and create its configuration:

```sh
export CAMERA_SERIAL=123456789
cp config/serial_number.example.json "config/${CAMERA_SERIAL}.json"
```

Edit `config/123456789.json`: set `camera.serial` to `123456789`,
`engine.backend` to `tensorrt`, and `engine.model_path` to `/models/yolo11n.onnx`.
The example uses native execution defaults, so these engine settings must be changed
for Docker. Compose uses the serial to name both the project and container
`cam-123456789`. Connect the selected RealSense camera to USB 3, then:

```sh
docker compose up --build -d
docker compose logs -f cam-node
```

The first image build downloads dependencies and compiles RealSense and `cam_node`;
it needs internet access but no GPU. Compose also passes `../rerun` (the shared
Rerun connection library) as an extra build context, which needs Docker Compose 2.17
or newer; build through Compose rather than with a plain `docker build`. The ONNX model is already in `models/`.
Startup selects the configured camera and starts `cam_node`, which builds an engine
on the selected GPU before detection starts. Engine compilation can take several
minutes. Subsequent starts reuse the cached engine. No graphical preview is configured.

`CAM_NODE_CONFIG` (default `config/${CAMERA_SERIAL}.json`) is mounted read-only
at the same relative path inside the container and must exist before starting it.
`CAMERA_SERIAL` overrides its camera serial. Each camera uses its own configuration
file. If you do not know the serial, build the image and list cameras using the
example config:

```sh
CAMERA_SERIAL=discovery docker compose build
CAMERA_SERIAL=discovery CAM_NODE_CONFIG=config/serial_number.example.json \
  docker compose run --rm cam-node list
```

Then set the discovered serial and create its configuration as shown above.

Compose accepts `CAMERA_SERIAL`, `GPU_DEVICE` (host GPU index or
UUID, default `0`), `BUILD_JOBS` (default `4`), `TENSORRT` (`ON` or `OFF`,
default `ON`; `OFF` builds only the OpenCV DNN backend), `RERUN` (`ON` or
`OFF`, default `ON`; `OFF` leaves the Rerun SDK out of the build), `LOG_LEVEL` (default `info`, see
[logging](configuration.md#logging)), and the optional
[Rerun settings](configuration.md#rerun-live-debugging) `RERUN_URL` and
`RERUN_IMAGE_FPS` from the shell or an untracked `.env` file beside
`compose.yaml`. With the viewer from `modules/rerun` on the same host, use
`RERUN_URL=rerun+http://host.docker.internal:9876/proxy`.

You can also supply the serial for an individual command:

```sh
CAMERA_SERIAL=123456789 docker compose up --build -d
```

Relative paths resolve from `compose.yaml`'s directory. All commands below
assume `CAMERA_SERIAL` is exported as shown above. If you previously exported
`COMPOSE_FILE` for the split configuration, run `unset COMPOSE_FILE` first.

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

Compose grants GPU access using an NVIDIA device reservation, mounts `/dev/bus/usb`,
and allows USB device major `189`. It does not use privileged mode. RealSense is
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
docker compose exec cam-node nvidia-smi
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
python3 -m unittest discover -s tests -p 'test_docker_entrypoint.py'
```
