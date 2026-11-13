FROM nvcr.io/nvidia/tensorrt:26.05-py3

# Every RUN after an ARG sees it, so a changed value rebuilds those layers.
# Each ARG is declared just before the first step that needs it.
ENV DEBIAN_FRONTEND=noninteractive

# Build dependencies pinned in the file shared with the native installer.
COPY scripts/ubuntu-24.04-packages.txt /tmp/ubuntu-24.04-packages.txt
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        $(sed 's/#.*//' /tmp/ubuntu-24.04-packages.txt) \
        ca-certificates git pkg-config python3 \
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
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
# Context from compose.yaml; same relative path as in the repo.
COPY --from=rerun . /src/rerun/
# ON builds the TensorRT backend. OFF leaves only OpenCV DNN.
ARG TENSORRT=ON
# ON links the Rerun SDK (used when RERUN_URL is set). OFF leaves it out.
ARG RERUN=ON
# build/ is a BuildKit cache kept between builds. Arrow and Rerun compile once.
# Later builds only recompile changed cam_node files. It is not in the image.
RUN --mount=type=cache,target=/src/cam_node/build,sharing=locked \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT="${TENSORRT}" \
        -DCAM_NODE_RERUN="${RERUN}" \
        -DCAM_NODE_OUTPUT_DIR=/opt/cam_node/bin \
    && cmake --build build --parallel "${BUILD_JOBS}"

COPY docker/entrypoint.py /opt/cam_node/entrypoint.py
WORKDIR /opt/cam_node
ENTRYPOINT ["python3", "-u", "/opt/cam_node/entrypoint.py"]
