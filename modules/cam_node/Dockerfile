# Plain Ubuntu, no NVIDIA base image. compose.cpu.yaml and compose.gpu.yaml pick
# the final stage with build.target. BuildKit builds only the stages it needs.
# - build-base: compilers and RealSense from source. Shared by both backends.
# - build-cpu, build-gpu: cam_node with or without TensorRT.
# - runtime-base: runtime libraries. Shared by both backends.
# - cpu, gpu: final images. gpu adds the CUDA and TensorRT runtime.
# - The GPU driver comes from the host through the NVIDIA Container Toolkit.

# CUDA and TensorRT from NVIDIA's APT repository. Keep the pair compatible.
# Declared here so build-gpu and gpu share one value.
ARG CUDA_VERSION=13.2
ARG TENSORRT_VERSION=10.16.1.11-1+cuda13.2
ARG CUDA_KEYRING_URL=https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb


FROM ubuntu:24.04 AS build-base

# Every RUN after an ARG sees it, so a changed value rebuilds those layers.
# Each ARG is declared just before the first step that needs it.
ENV DEBIAN_FRONTEND=noninteractive

# Build dependencies pinned in the file shared with the native installer.
COPY scripts/ubuntu-24.04-packages.txt /tmp/ubuntu-24.04-packages.txt
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        $(sed 's/#.*//' /tmp/ubuntu-24.04-packages.txt) \
        ca-certificates git pkg-config \
        libusb-1.0-0-dev libudev-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/* /tmp/ubuntu-24.04-packages.txt

ARG REALSENSE_VERSION=2.58.4
# Parallel compile jobs. Each can use 1-2 GB RAM. Too many freeze small hosts.
ARG BUILD_JOBS=2

# Userspace USB backend: no RealSense kernel modules are installed on the host.
RUN git clone --depth 1 --branch "v${REALSENSE_VERSION}" \
        https://github.com/realsenseai/librealsense.git /tmp/librealsense \
    && cmake -S /tmp/librealsense -B /tmp/librealsense/build \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON -DFORCE_RSUSB_BACKEND=ON \
        -DBUILD_EXAMPLES=OFF -DBUILD_GRAPHICAL_EXAMPLES=OFF \
        -DBUILD_GLSL_EXTENSIONS=OFF -DBUILD_TOOLS=OFF \
        -DBUILD_PYTHON_BINDINGS=OFF -DBUILD_WITH_DDS=OFF \
        -DBUILD_ROSBAG2=OFF -DCHECK_FOR_UPDATES=OFF \
        -DUSE_EXTERNAL_NLOHMANN_JSON=ON \
    && cmake --build /tmp/librealsense/build --parallel "${BUILD_JOBS}" \
    && cmake --install /tmp/librealsense/build \
    && install -Dm644 /tmp/librealsense/config/99-realsense-libusb.rules \
        /opt/cam_node/99-realsense-libusb.rules \
    && ldconfig \
    && rm -rf /tmp/librealsense

WORKDIR /src/cam_node


# cam_node with OpenCV DNN only.
FROM build-base AS build-cpu

COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
# Context from compose.cpu.yaml; same relative path as in the repo.
COPY --from=rerun . /src/rerun/
ARG BUILD_JOBS=2
# ON links the Rerun SDK (used when RERUN_URL is set). OFF leaves it out.
ARG RERUN=ON
# build/ is a BuildKit cache kept between builds. Arrow and Rerun compile once.
# Later builds only recompile changed cam_node files. It is not in the image.
# Each backend has its own cache id, so switching does not reconfigure the other.
RUN --mount=type=cache,id=cam-node-build-cpu,target=/src/cam_node/build,sharing=locked \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT=OFF \
        -DCAM_NODE_RERUN="${RERUN}" \
        -DCAM_NODE_BUILD_TESTS=OFF \
        -DCAM_NODE_OUTPUT_DIR=/opt/cam_node/bin \
    && cmake --build build --parallel "${BUILD_JOBS}"


# cam_node with the TensorRT backend.
FROM build-base AS build-gpu

ARG CUDA_VERSION
ARG TENSORRT_VERSION
ARG CUDA_KEYRING_URL
ADD ${CUDA_KEYRING_URL} /tmp/cuda-keyring.deb
# Headers and link libraries for the TensorRT backend.
# - Installed before the sources are copied, so source edits do not reinstall them.
# - nvcc is only here so CMake finds the CUDA toolkit.
# - The -dev packages are several GB. They stay in this stage.
RUN cuda="$(echo "${CUDA_VERSION}" | tr . -)" \
    && dpkg -i /tmp/cuda-keyring.deb \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        "cuda-nvcc-${cuda}" "cuda-cudart-dev-${cuda}" \
        "libnvinfer10=${TENSORRT_VERSION}" \
        "libnvinfer-headers-dev=${TENSORRT_VERSION}" \
        "libnvinfer-safe-headers-dev=${TENSORRT_VERSION}" \
        "libnvinfer-dev=${TENSORRT_VERSION}" \
        "libnvonnxparsers10=${TENSORRT_VERSION}" \
        "libnvonnxparsers-dev=${TENSORRT_VERSION}" \
    && rm -rf /var/lib/apt/lists/* /tmp/cuda-keyring.deb
ENV PATH="/usr/local/cuda-${CUDA_VERSION}/bin:${PATH}"

COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
# Context from compose.cpu.yaml; same relative path as in the repo.
COPY --from=rerun . /src/rerun/
ARG BUILD_JOBS=2
# ON links the Rerun SDK (used when RERUN_URL is set). OFF leaves it out.
ARG RERUN=ON
# Same BuildKit cache as build-cpu, under its own id.
RUN --mount=type=cache,id=cam-node-build-gpu,target=/src/cam_node/build,sharing=locked \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT=ON \
        -DCAM_NODE_RERUN="${RERUN}" \
        -DCAM_NODE_BUILD_TESTS=OFF \
        -DCAM_NODE_OUTPUT_DIR=/opt/cam_node/bin \
    && cmake --build build --parallel "${BUILD_JOBS}"


# Runtime libraries of the build dependencies. python3 runs the entrypoint.
FROM ubuntu:24.04 AS runtime-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates python3 \
        libopencv-core406t64 libopencv-imgproc406t64 libopencv-dnn406t64 \
        libfmt9 libusb-1.0-0 libudev1 libssl3t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/cam_node
ENTRYPOINT ["python3", "-u", "/opt/cam_node/entrypoint.py"]


# Final CPU image. Selected by compose.cpu.yaml.
FROM runtime-base AS cpu

# RealSense libraries, cam_node and the udev rules.
COPY --from=build-cpu /usr/local/lib/ /usr/local/lib/
COPY --from=build-cpu /opt/cam_node/ /opt/cam_node/
RUN ldconfig
COPY docker/entrypoint.py /opt/cam_node/entrypoint.py


# Final GPU image. Selected by compose.gpu.yaml.
FROM runtime-base AS gpu

ARG CUDA_VERSION
ARG TENSORRT_VERSION
ARG CUDA_KEYRING_URL
ADD ${CUDA_KEYRING_URL} /tmp/cuda-keyring.deb
# TensorRT runtime, ONNX parser and the CUDA runtime library. Nothing else.
RUN cuda="$(echo "${CUDA_VERSION}" | tr . -)" \
    && dpkg -i /tmp/cuda-keyring.deb \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        "cuda-cudart-${cuda}" \
        "libnvinfer10=${TENSORRT_VERSION}" \
        "libnvonnxparsers10=${TENSORRT_VERSION}" \
    && rm -rf /var/lib/apt/lists/* /tmp/cuda-keyring.deb

# Read by the NVIDIA Container Toolkit. The NVIDIA base image used to set them.
ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility

# RealSense libraries, cam_node and the udev rules.
COPY --from=build-gpu /usr/local/lib/ /usr/local/lib/
COPY --from=build-gpu /opt/cam_node/ /opt/cam_node/
RUN ldconfig
COPY docker/entrypoint.py /opt/cam_node/entrypoint.py
