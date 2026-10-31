# GPU setup

The GPU installer supports Ubuntu 22.04/24.04 x86-64 and TensorRT 10. Install a compatible NVIDIA driver first using [Ubuntu's driver installation guide](https://ubuntu.com/server/docs/nvidia-drivers-installation/). Verify it with `nvidia-smi` before continuing.

Configure [NVIDIA's CUDA APT repository](https://developer.nvidia.com/cuda-downloads) for your Ubuntu release so the packages can be found. Follow the repository setup steps; this script installs the toolkit separately.

List available versions:

```sh
apt-cache search '^cuda-toolkit-[0-9]'
apt-cache madison libnvinfer-dev
```

Choose a compatible CUDA/TensorRT pair from the [NVIDIA support matrix](https://docs.nvidia.com/deeplearning/tensorrt/10.x.x/getting-started/support-matrix.html). Pass the CUDA suffix (such as `12-8`) and the full TensorRT APT version, including its CUDA suffix:

```sh
./scripts/install-gpu-deps.sh --cuda-version <major-minor> --tensorrt-version <apt-version>
```

The installer requests exact TensorRT package versions and a CUDA toolkit release. It leaves driver installation to the machine setup. Keep these version arguments for repeat deployments.
