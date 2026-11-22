#!/usr/bin/env bash

# Download the YOLO11 pose models and export them to ONNX in models/:
# - Sizes n, s, m, l, x at input sizes 640, 960 and 1280.
# - Writes models/yolo11<size>-pose-<imgsz>.onnx. Set input_size to the same <imgsz>.

set -euo pipefail

cd "$(dirname -- "${BASH_SOURCE[0]}")/../models"

pip install ultralytics

# Sizes n, s, m, l, x at input sizes 640, 960 and 1280.
for size in n s m l x; do
    for imgsz in 640 960 1280; do
        yolo export model=yolo11${size}-pose.pt format=onnx imgsz=${imgsz}
        mv yolo11${size}-pose.onnx yolo11${size}-pose-${imgsz}.onnx
    done
done
