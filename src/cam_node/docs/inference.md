# Inference

Multiple models were tested to compare requirements and results. The models were run on the CPU first for validating the output tensor and the general end-to-end flow.

## CPU implementation

[OpenCV DNN](https://opencv.org/opencv-dnn-module/) loads ONNX models and runs inference on the CPU. The example uses `models/yolo11n.onnx` with `backend` set to `opencv_dnn`.

## Model requirements

Both backends accept `RGB8` or `BGR8` color frames, normalize pixels to 0–1, pad the image to a square and resize to `input_size`.

Use a standard COCO YOLOv8/YOLO11 detection export with output `[1, 84, N]`, without embedded NMS. The engines select people (class 0) and apply the configured confidence and NMS thresholds. Pose models are not supported.

## GPU acceleration

**TensorRT** loads `.engine` plans on an NVIDIA GPU. CUDA and TensorRT 10 or newer must already be installed:

```sh
cmake -S . -B build -DTENSORRT=ON -DTensorRT_ROOT=/path/to/TensorRT
cmake --build build --parallel
```

Build the plan offline on the deployment GPU using NVIDIA's `trtexec`:

```sh
trtexec --onnx=models/yolo11n.onnx --saveEngine=models/yolo11n.engine
```

For a dynamic ONNX input, also supply `--shapes=images:1x3x640x640` (replace `images` with the model's input name).

Set `backend` to `tensor_rt`, `model_path` to the resulting plan, and `input_size` to its square image size.

The backend expects one FP32 linear NCHW RGB input and one FP32 output `[1, 84, N]`. Dynamic plans use optimization profile 0, which must support the configured input shape. FP16 input/output and data-dependent output shapes are unsupported.

The runtime deserializes a plan, binds reusable device buffers by tensor name and uses `enqueueV3` with a CUDA stream. Only load plans from trusted sources; plans must be compatible with the deployment GPU, platform and TensorRT version. Use a separate engine instance for each worker; concurrent calls to the same instance are unsupported.
