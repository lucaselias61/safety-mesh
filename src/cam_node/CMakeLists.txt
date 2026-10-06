find_package(realsense2 REQUIRED)

add_executable(cam_node
    src/main.cpp
    src/camera.cpp
    src/discovery.cpp
    src/stream.cpp
)

target_include_directories(cam_node PRIVATE include)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(cam_node PRIVATE fmt::fmt)
target_link_libraries(cam_node PRIVATE safety_mesh_version realsense2::realsense2)
