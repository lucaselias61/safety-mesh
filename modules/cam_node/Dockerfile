FROM nvcr.io/nvidia/tensorrt:26.05-py3

ARG REALSENSE_VERSION=2.58.4
ARG BUILD_JOBS=4
ENV DEBIAN_FRONTEND=noninteractive

# Build dependencies pinned in the file shared with the native installer.
COPY scripts/ubuntu-24.04-packages.txt /tmp/ubuntu-24.04-packages.txt
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        $(sed 's/#.*//' /tmp/ubuntu-24.04-packages.txt) \
        ca-certificates git pkg-config python3 \
        libusb-1.0-0-dev libudev-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/* /tmp/ubuntu-24.04-packages.txt

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
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTENSORRT=ON \
        -DCAM_NODE_OUTPUT_DIR=/opt/cam_node/bin \
    && cmake --build build --parallel "${BUILD_JOBS}" \
    && rm -rf /src/cam_node

COPY docker/entrypoint.py /opt/cam_node/entrypoint.py
WORKDIR /opt/cam_node
ENTRYPOINT ["python3", "-u", "/opt/cam_node/entrypoint.py"]
