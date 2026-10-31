# Troubleshooting

| Problem | Check |
| --- | --- |
| Camera missing or unavailable | Run `./bin/cam_node list`. Check USB connection, permission rules and whether another process is using the camera. |
| Stream fails to start | Use `streams --serial <serial>` to check that format, resolution and FPS are supported. |
| Invalid configuration | Check the file path and required fields in [configuration](configuration.md). Model paths resolve from the working directory. |
| No detections | Check that color is enabled in `RGB8` or `BGR8`, the model is compatible, and the confidence threshold is suitable. |
| `Expected YOLO detection output [1, 84, N]` | Use a standard COCO YOLOv8/YOLO11 detection model without embedded NMS. Pose exports are unsupported. |
| TensorRT backend unavailable | Reconfigure with `-DTENSORRT=ON` and rebuild. See [inference](inference.md). |
| No frames received | The worker retries, then stops after five consecutive missing frames. Check the camera connection and stream profiles. |
| Preview window fails | `view` requires a graphical display and OpenCV window support. |
| Dependency or build failure | Run the [setup](setup.md) installer on a supported Ubuntu release. RealSense must be 2.58 or newer. |
