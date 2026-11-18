# Unit tests. No camera, no GPU.
# Paths are relative to this file, so the module also builds as a subproject.

# Renders its own test board.
# world_pose.cpp is built only here until a command uses it. Adding it to
# cam_node also needs the calib3d and aruco runtime packages in the images.
add_executable(cam_node_world_pose_test
    world_pose_test.cpp
    ../../src/world_pose.cpp
)
target_compile_features(cam_node_world_pose_test PRIVATE cxx_std_17)
target_include_directories(cam_node_world_pose_test PRIVATE
    ../../include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cam_node_world_pose_test PRIVATE
    nlohmann_json::nlohmann_json realsense2::realsense2 ${OpenCV_LIBS})
target_compile_definitions(cam_node_world_pose_test PRIVATE
    CAM_NODE_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../config")
add_test(NAME cam_node_world_pose COMMAND cam_node_world_pose_test)
