# Inference

Multiple models were tested to compare requirements and results. The models were run on the CPU first for validating the output tensor and the general end-to-end flow.

## CPU implementation

[OpenCV DNN](https://opencv.org/opencv-dnn-module/) loads ONNX models and runs inference on the CPU. The example uses `models/yolo11n.onnx` with `backend` set to `opencv_dnn`.

## Model requirements

Both backends accept `RGB8` or `BGR8` color frames, normalize pixels to 0–1, pad the image to a square and resize to `input_size`.

Use a standard COCO YOLOv8/YOLO11 export without embedded NMS, either detection or pose. The engines pick the model type from the output shape:

| Model | Output | Result |
| --- | --- | --- |
| Detection (`yolo11n.onnx`) | `[1, 84, N]` | Person boxes (class 0) |
| Pose (`yolo11n-pose.onnx`) | `[1, 56, N]` | Person boxes and 17 COCO keypoints each |

Both apply the configured confidence and NMS thresholds to the person score. Pose detections always carry all 17 keypoints in COCO order (nose, eyes, ears, shoulders, elbows, wrists, hips, knees, ankles), each with a confidence. Keypoints below 0.5 confidence were guessed by the model, for example when occluded, and are not shown in the viewer.

Export a pose model with `yolo export model=yolo11n-pose.pt format=onnx imgsz=640`.

## GPU acceleration

For a tar installation, add `--tensorrt-root /path/to/TensorRT` to the build command.

Set `backend` to `tensorrt`, `model_path` to the ONNX model, and `input_size` to its square image size. On first run, `cam_node` builds an engine for the current GPU and caches it in `.engines/` next to the model, with a `.json` file describing it. Later runs reuse it. The cache key covers the model contents, input size, GPU, CUDA and TensorRT versions, so changing any of them builds a new engine. The first build can take several minutes; processes sharing the directory wait for each other, so each engine is built once. A shutdown request cancels a build in progress.

Paths that do not end in `.onnx` are loaded as prebuilt plans and are not cached or checked. Build such plans on the deployment GPU, with the same TensorRT version as `cam_node`.

The backend expects one FP32 linear NCHW RGB input and one FP32 output `[1, 84, N]` or `[1, 56, N]`. Dynamic plans use optimization profile 0, which must support the configured input shape. FP16 input/output and data-dependent output shapes are unsupported.

The runtime deserializes a plan, binds reusable device buffers by tensor name and uses `enqueueV3` with a CUDA stream. Only load plans from trusted sources; plans must be compatible with the deployment GPU, platform and TensorRT version. Use a separate engine instance for each worker; concurrent calls to the same instance are unsupported.

