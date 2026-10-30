# Inference 

Multiple models were tested to compare requirements and results. The models were run on the CPU first for validating the output tensor and the general end-to-end flow.

## Engines

### CPU implementation

- **OpenCV's DNN Module:** OpenCV loads ONNX models and exposes inference APIs. [More about the module](https://opencv.org/opencv-dnn-module/)


### GPU Accelaration

- **TensorRT:** Models in the '.engine' format are loaded into a Nvidia GPU. Multi-instance GPU (MIG) deployment for dedicated GPU slices, it virtualy turns one GPU into multiple smaller ones for parallel work.

## Modules 
