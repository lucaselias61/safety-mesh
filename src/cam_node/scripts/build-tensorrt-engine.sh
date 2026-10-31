#!/usr/bin/env bash
set -euo pipefail

usage() { echo "Usage: $0 --onnx MODEL --output ENGINE [--shapes images:1x3x640x640]"; }
onnx=""
output=""
shapes=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --onnx|--output|--shapes)
            [[ $# -ge 2 && -n "$2" ]] || { usage >&2; exit 1; }
            case "$1" in
                --onnx) onnx="$2" ;; --output) output="$2" ;; --shapes) shapes="$2" ;;
            esac
            shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) usage >&2; exit 1 ;;
    esac
done
[[ -f "$onnx" && -n "$output" ]] || { usage >&2; exit 1; }
[[ ! -e "$output" && ! -e "$output.meta.txt" ]] || { echo "Output already exists: $output" >&2; exit 1; }
trtexec="${TRTEXEC:-trtexec}"
if ! command -v "$trtexec" >/dev/null; then
    if [[ -x /usr/src/tensorrt/bin/trtexec ]]; then trtexec=/usr/src/tensorrt/bin/trtexec
    else echo "trtexec not found. Install GPU dependencies or set TRTEXEC to its path." >&2; exit 1; fi
fi
command -v nvidia-smi >/dev/null || { echo "Requires an NVIDIA deployment GPU." >&2; exit 1; }
nvidia-smi >/dev/null
mkdir -p -- "$(dirname -- "$output")"
options=("--onnx=$onnx" "--saveEngine=$output" --skipInference)
[[ -z "$shapes" ]] || options+=("--shapes=$shapes")
"$trtexec" "${options[@]}"
{
    date -u '+built=%Y-%m-%dT%H:%M:%SZ'
    echo "onnx=$onnx"
    sha256sum -- "$onnx"
    echo "shapes=${shapes:-static model shape}"
    nvidia-smi --query-gpu=name,uuid,driver_version --format=csv,noheader
    "$trtexec" --version
} > "$output.meta.txt"
