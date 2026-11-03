# RealSense devices

[librealsense](https://github.com/realsenseai/librealsense) provides access to RealSense stereo depth products, like the D435i or D455.

RealSense also has a ROS wrapper, but for simplicity we use direct communication between the modules. Each module has a well defined interface and does not need a ROS middleware layer. Adding it would add dependencies and deployment complexity.

## Streams and formats

Available streams depend on the device. The example configuration uses:

| Stream | Format | Purpose |
| --- | --- | --- |
| Color | `RGB8` | Color image for inference. `BGR8` is also accepted by the engines. |
| Depth | `Z16` | 16-bit depth values; meters = pixel value × depth scale. |

Other SDK streams include infrared, gyro, accelerometer and pose; not every device provides them. List the connected camera's supported profiles with the [streams command](usage.md#list-stream-profiles).

The [SDK header](https://github.com/realsenseai/librealsense/blob/master/include/librealsense2/h/rs_sensor.h) contains the full stream and format enums.

USB permission rules are installed during [setup](setup.md). The current detection worker uses color frames; depth is not yet used to produce 3D detections.
