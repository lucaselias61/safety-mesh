# Rerun

Shared [Rerun](https://rerun.io) viewer for live debugging. `cam_node` and
`manager` containers stream images, detections, robot keypoints and latency
to it, and all of them appear on one timeline. Monitoring only: nothing on
the safety path depends on it.

This module holds both the viewer container (`compose.yaml`, `Dockerfile`)
and `rerun_logger`, the C++ library nodes use to connect to it
(`CMakeLists.txt`, `include/`, `src/`, `cmake/`).

## Viewer

```sh
docker network create safety-mesh   # once per host
docker compose up -d --build
```

- Web viewer: `http://<host>:9090`
- SDK endpoint: `rerun+http://host.docker.internal:9876/proxy` from
  containers on this host (cam_node's default setup),
  `rerun+http://rerun:9876/proxy` from containers on the `safety-mesh`
  network, `rerun+http://<host>:9876/proxy` from other machines
- Native viewer: `rerun --connect rerun+http://<host>:9876/proxy`

`RERUN_MEMORY_LIMIT` (default `2GB`) caps the data buffered for late viewers;
the oldest data is dropped past it. Keep `RERUN_VERSION` in
`Dockerfile` equal to `RERUN_SDK_VERSION` in `cmake/Rerun.cmake`.

## Logger library

`rerun_logger::ViewerConnection` holds what every node shares: reading
`RERUN_URL` and `RERUN_IMAGE_FPS`, connecting to the shared `safety_mesh`
recording, entity paths under a per-node root (e.g. `cams/<serial>`), the
`frame` and `capture_time` timelines and the image rate limit. It knows
nothing about cameras or robots; each node converts its own types to Rerun
archetypes (cam_node: `src/viewer_sink.cpp`).

```cmake
if(NOT TARGET rerun_logger)
    add_subdirectory(../rerun ${CMAKE_BINARY_DIR}/rerun_logger)
endif()
target_link_libraries(my_node PRIVATE rerun_logger)
```

The first configure downloads the Rerun C++ SDK and builds its Arrow
dependency, which takes a few minutes.
