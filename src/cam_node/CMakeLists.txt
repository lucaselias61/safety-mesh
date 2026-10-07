cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cam_node LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(realsense2 REQUIRED)

add_executable(cam_node
    src/main.cpp
    src/discovery.cpp
    src/camera.cpp
    src/stream.cpp
)

target_compile_features(cam_node PRIVATE cxx_std_17)
set_target_properties(cam_node PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(cam_node PRIVATE include)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(cam_node PRIVATE fmt::fmt)
target_link_libraries(cam_node PRIVATE realsense2::realsense2)
