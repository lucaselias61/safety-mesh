"""Logs RealSense framesets to Rerun the way cam_node does (src/viewer_sink.cpp).

- Paths: cams/<serial>/color and cams/<serial>/depth.
- Timelines: frame, and capture_time for epoch based timestamps.
"""

import numpy as np
import pyrealsense2 as rs
import rerun as rr

# Same as APP_ID in rerun/tools/blueprint.py, so its layout applies.
APP_ID = "safety_mesh"
# Same as depth_color_range_m in include/viewer_sink.hpp.
DEPTH_COLOR_RANGE_M = 6.0
# Timestamp domains that are milliseconds since the epoch.
EPOCH_DOMAINS = (rs.timestamp_domain.global_time, rs.timestamp_domain.system_time)
COLOR_MODELS = {
    rs.format.rgb8: "RGB",
    rs.format.bgr8: "BGR",
    rs.format.rgba8: "RGBA",
    rs.format.bgra8: "BGRA",
}


def open_file(path):
    """Returns a recording stream that writes to path.

    - Own recording id per file, so it does not mix with live cam_node data.
    """
    rec = rr.RecordingStream(APP_ID, recording_id=f"rec_stream/{path.name}")
    rec.save(str(path))
    return rec


def log_profile(rec, profile):
    """Logs intrinsics and the depth-to-color transform of a started pipeline.

    Returns the root path, cams/<serial>.
    """
    root = f"cams/{profile.get_device().get_info(rs.camera_info.serial_number)}"
    color = video_profile(profile, rs.stream.color)
    depth = video_profile(profile, rs.stream.depth)
    warn_unsupported(color, depth)
    # Same as ViewerSink::calibration. Color is the root frame.
    if color:
        rec.log(f"{root}/color", pinhole(color.get_intrinsics()), static=True)
    if depth:
        archetypes = [pinhole(depth.get_intrinsics())]
        if color:
            archetypes.insert(0, transform(depth.get_extrinsics_to(color)))
        rec.log(f"{root}/depth", *archetypes, static=True)
    return root


def log_frameset(rec, root, index, frameset):
    color = frameset.get_color_frame()
    depth = frameset.get_depth_frame()
    reference = color or depth
    if not reference:
        return
    rec.set_time("frame", sequence=index)
    # Hardware clock timestamps are not epoch based. Those get only the frame timeline.
    if reference.get_frame_timestamp_domain() in EPOCH_DOMAINS:
        rec.set_time("capture_time", timestamp=reference.get_timestamp() / 1000)
    if color:
        model = COLOR_MODELS.get(color.get_profile().format())
        if model:
            rec.log(f"{root}/color", rr.Image(np.asanyarray(color.get_data()), color_model=model))
    if depth and depth.get_profile().format() == rs.format.z16:
        units_per_meter = 1.0 / depth.get_units()
        rec.log(f"{root}/depth", rr.DepthImage(
            np.asanyarray(depth.get_data()),
            meter=units_per_meter,
            depth_range=(0.0, DEPTH_COLOR_RANGE_M * units_per_meter),
        ))


def video_profile(profile, stream_type):
    # None if the pipeline has no such stream.
    for stream in profile.get_streams():
        if stream.stream_type() == stream_type:
            return stream.as_video_stream_profile()
    return None


def pinhole(intrinsics):
    return rr.Pinhole(
        focal_length=[intrinsics.fx, intrinsics.fy],
        principal_point=[intrinsics.ppx, intrinsics.ppy],
        width=intrinsics.width,
        height=intrinsics.height,
        camera_xyz=rr.ViewCoordinates.RDF,
    )


def transform(extrinsics):
    # rs2 rotation is column-major.
    r = extrinsics.rotation
    rows = [[r[col * 3 + row] for col in range(3)] for row in range(3)]
    return rr.Transform3D(translation=extrinsics.translation, mat3x3=rows)


def warn_unsupported(color, depth):
    if color and color.format() not in COLOR_MODELS:
        print(f"Color format {color.format()} is not RGB or BGR. Not logging color.", flush=True)
    if depth and depth.format() != rs.format.z16:
        print(f"Depth format {depth.format()} is not Z16. Not logging depth.", flush=True)
