# Live tests. Need the configured RealSense camera connected.
# Every test here gets the live label: run with ctest -L live, skip with -LE live.
set_property(DIRECTORY PROPERTY LABELS live)

add_executable(cam_node_cam_pipeline_test cam_pipeline_test.cpp)
target_link_libraries(cam_node_cam_pipeline_test PRIVATE cam_node_core)
add_test(NAME cam_node_cam_pipeline COMMAND cam_node_cam_pipeline_test)
# Start, global time settling and the measuring window take about 10 s.
set_tests_properties(cam_node_cam_pipeline PROPERTIES TIMEOUT 120)
