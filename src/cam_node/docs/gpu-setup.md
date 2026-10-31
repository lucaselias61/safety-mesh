# GPU setup

The current backend targets TensorRT 10 and rejects older versions. Pascal GPUs, such as the GTX 1050 Ti, are not supported by TensorRT 10. Use the CPU backend on those GPUs; TensorRT 8.6 support would require code changes and testing.

Check your GPU against the [TensorRT support matrix](https://docs.nvidia.com/deeplearning/tensorrt/10.x.x/getting-started/support-matrix.html) before installing dependencies. The CUDA version shown by `nvidia-smi` describes driver capability, not the installed toolkit.

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
