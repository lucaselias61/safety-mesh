# Unit tests. No camera, no GPU.

# Renders its own test board.
add_executable(cam_node_world_pose_test world_pose_test.cpp)
target_link_libraries(cam_node_world_pose_test PRIVATE
    cam_node_world_pose ${OpenCV_LIBS})
target_compile_definitions(cam_node_world_pose_test PRIVATE
    CAM_NODE_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../config")
add_test(NAME cam_node_world_pose COMMAND cam_node_world_pose_test)
