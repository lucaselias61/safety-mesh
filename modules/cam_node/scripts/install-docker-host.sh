#!/usr/bin/env bash

# Check the environment and prepare a host for Docker deployment:
# - Docker Engine and the Compose plugin from Docker's APT repository.
# - NVIDIA Container Toolkit from NVIDIA's APT repository.
# - Docker configured to use the NVIDIA runtime, then a GPU container check.

# Requires Ubuntu 24.04 or newer (x86-64), a working NVIDIA driver,
# and root or sudo access. Versions are pinned below.
# Sources:
# https://docs.docker.com/engine/install/ubuntu/
# https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html

set -euo pipefail

docker_version=29.9.3
nvidia_container_toolkit_version=1.20.1-1
check_image=ubuntu:24.04

usage() {
    echo "Usage: $0"
    echo "Requires a working NVIDIA driver. Restarts Docker if its runtime configuration changes."
}
case "${1:-}" in
    "") ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
esac

[[ "$(uname -s)" == Linux ]] || { echo "Requires Ubuntu Linux." >&2; exit 1; }
[[ -r /etc/os-release ]] || { echo "Cannot identify the Linux distribution." >&2; exit 1; }
. /etc/os-release
if [[ "${ID:-}" != ubuntu || "$(uname -m)" != x86_64 || ! "${VERSION_ID:-}" =~ ^[0-9]+\.[0-9]+$ ]] ||
    ! dpkg --compare-versions "$VERSION_ID" ge 24.04; then
    echo "Docker host installer requires Ubuntu 24.04 or newer, x86-64." >&2
    exit 1
fi
codename="${UBUNTU_CODENAME:-${VERSION_CODENAME:-}}"
[[ -n "$codename" ]] || { echo "Cannot identify the Ubuntu repository codename." >&2; exit 1; }
command -v nvidia-smi >/dev/null && nvidia-smi >/dev/null || {
    echo "Install a compatible NVIDIA driver first and verify it with nvidia-smi." >&2; exit 1;
}

# Docker's packages conflict with these. Removing them can stop existing
# containers, so ask instead of removing them here.
conflicts=()
for package in docker.io docker-compose docker-compose-v2 docker-doc docker-buildx podman-docker containerd runc; do
    if dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q 'install ok installed'; then
        conflicts+=("$package")
    fi
done
if [[ ${#conflicts[@]} -gt 0 ]]; then
    echo "Remove conflicting packages first: sudo apt-get remove ${conflicts[*]}" >&2
    exit 1
fi

root_cmd=()
if [[ "$(id -u)" != 0 ]]; then
    command -v sudo >/dev/null || { echo "Install sudo or run this installer as root." >&2; exit 1; }
    root_cmd=(sudo)
fi
# Use a nonempty command array for compatibility with Bash 3 and set -u.
root_cmd+=(env)

"${root_cmd[@]}" apt-get update
"${root_cmd[@]}" apt-get install -y --no-install-recommends ca-certificates curl gnupg

key_dir="$(mktemp -d)"
trap 'rm -rf "$key_dir"' EXIT
"${root_cmd[@]}" install -d -m 0755 /etc/apt/keyrings

# Docker repository, in the deb822 format Docker documents.
curl --fail --silent --show-error --location \
    https://download.docker.com/linux/ubuntu/gpg -o "$key_dir/docker.asc"
"${root_cmd[@]}" install -m 0644 "$key_dir/docker.asc" /etc/apt/keyrings/docker.asc
printf '%s\n' \
    "Types: deb" \
    "URIs: https://download.docker.com/linux/ubuntu" \
    "Suites: $codename" \
    "Components: stable" \
    "Architectures: $(dpkg --print-architecture)" \
    "Signed-By: /etc/apt/keyrings/docker.asc" |
    "${root_cmd[@]}" tee /etc/apt/sources.list.d/docker.sources >/dev/null

# NVIDIA Container Toolkit repository.
curl --fail --silent --show-error --location \
    https://nvidia.github.io/libnvidia-container/gpgkey -o "$key_dir/nvidia.asc"
gpg --batch --dearmor --output "$key_dir/nvidia.gpg" "$key_dir/nvidia.asc"
"${root_cmd[@]}" install -m 0644 "$key_dir/nvidia.gpg" \
    /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl --fail --silent --show-error --location \
    https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list |
    sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' |
    "${root_cmd[@]}" tee /etc/apt/sources.list.d/nvidia-container-toolkit.list >/dev/null

"${root_cmd[@]}" apt-get update
docker_package_version="5:${docker_version}-1~ubuntu.${VERSION_ID}~${codename}"
packages=(
    "docker-ce=$docker_package_version"
    "docker-ce-cli=$docker_package_version"
    containerd.io
    docker-buildx-plugin
    docker-compose-plugin
)
for package in nvidia-container-toolkit nvidia-container-toolkit-base libnvidia-container-tools libnvidia-container1; do
    packages+=("$package=$nvidia_container_toolkit_version")
done
# Resolve everything before installing; fails if a pinned version is unavailable.
"${root_cmd[@]}" apt-get --simulate install "${packages[@]}"
"${root_cmd[@]}" apt-get install -y "${packages[@]}"
"${root_cmd[@]}" systemctl enable --now docker.service containerd.service

# nvidia-ctk edits /etc/docker/daemon.json. Restart Docker only if it changed,
# because a restart interrupts running containers.
daemon_config=/etc/docker/daemon.json
checksum() { "${root_cmd[@]}" cat "$daemon_config" 2>/dev/null | sha256sum || true; }
before="$(checksum)"
"${root_cmd[@]}" nvidia-ctk runtime configure --runtime=docker
if [[ "$(checksum)" != "$before" ]]; then
    echo "Docker runtime configuration changed; restarting Docker."
    "${root_cmd[@]}" systemctl restart docker
fi

"${root_cmd[@]}" docker run --rm --runtime=nvidia --gpus all "$check_image" nvidia-smi
echo "Docker host ready. See docs/docker.md to start cam_node."
echo "Optional: sudo usermod -aG docker \"\$USER\" runs Docker without sudo, but grants root-level access."
