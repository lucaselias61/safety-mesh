cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cam_node LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(realsense2 2.58 CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc)

add_executable(cam_node
    src/main.cpp
    src/worker.cpp
    src/camera.cpp
    src/pipeline.cpp
    src/vision.cpp
    src/cli.cpp
    src/stream.cpp
    src/config.cpp
)

set_target_properties(cam_node PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

target_compile_features(cam_node PRIVATE cxx_std_17)
set_target_properties(cam_node PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(cam_node PRIVATE include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cam_node PRIVATE
    CLI11::CLI11
    nlohmann_json::nlohmann_json
    fmt::fmt
    realsense2::realsense2
    ${OpenCV_LIBS}
)
