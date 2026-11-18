cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cam_node LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(realsense2 2.58 CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc dnn calib3d aruco)

option(TENSORRT "Enable the TensorRT 10+ GPU backend" OFF)
option(CAM_NODE_RERUN "Stream to a Rerun viewer when RERUN_URL is set" ON)
set(CAM_NODE_RERUN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rerun" CACHE PATH
    "Shared Rerun connection library")

# Camera side: discovery, config, streams, pipeline, timing, geometry.
# Shared by cam_node and the live tests, so they run the same code.
add_library(cam_node_core STATIC
    src/camera.cpp
    src/stream.cpp
    src/pipeline.cpp
    src/frame_time.cpp
    src/geometry.cpp
    src/utils.cpp
)
target_compile_features(cam_node_core PUBLIC cxx_std_17)
set_target_properties(cam_node_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(cam_node_core PUBLIC include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cam_node_core PUBLIC
    nlohmann_json::nlohmann_json
    realsense2::realsense2
    opencv_core
)

add_executable(cam_node
    src/main.cpp
    src/worker.cpp
    src/cli/list.cpp
    src/cli/run.cpp
    src/cli/streams.cpp
    src/logger.cpp
    src/engines/opencv_dnn_engine.cpp
    src/engines/yolo_decoder.cpp
)

set(CAM_NODE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Executable output directory")
set_target_properties(cam_node PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CAM_NODE_OUTPUT_DIR}")

set_target_properties(cam_node PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(cam_node PRIVATE
    cam_node_core
    CLI11::CLI11
    fmt::fmt
    # Only what cam_node uses. ${OpenCV_LIBS} would also link calib3d and aruco,
    # which the runtime images don't install.
    opencv_imgproc opencv_dnn
)

if(CAM_NODE_RERUN)
    if(NOT TARGET rerun_logger) # May already be added by manager.
        add_subdirectory("${CAM_NODE_RERUN_DIR}" "${CMAKE_BINARY_DIR}/rerun_logger")
    endif()
    target_sources(cam_node PRIVATE src/viewer_sink.cpp)
    target_link_libraries(cam_node PRIVATE rerun_logger)
    target_compile_definitions(cam_node PRIVATE CAM_NODE_WITH_RERUN=1)
endif()

if(TENSORRT)
    find_package(CUDAToolkit REQUIRED)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h
        HINTS ${TensorRT_ROOT} ENV TensorRT_ROOT PATH_SUFFIXES include)
    find_library(TENSORRT_LIBRARY nvinfer
        HINTS ${TensorRT_ROOT} ENV TensorRT_ROOT PATH_SUFFIXES lib lib64)
    find_library(TENSORRT_ONNX_PARSER_LIBRARY nvonnxparser
        HINTS ${TensorRT_ROOT} ENV TensorRT_ROOT PATH_SUFFIXES lib lib64)
    if(NOT TENSORRT_INCLUDE_DIR OR NOT TENSORRT_LIBRARY OR NOT TENSORRT_ONNX_PARSER_LIBRARY)
        message(FATAL_ERROR "TensorRT headers, nvinfer and nvonnxparser are required; set TensorRT_ROOT")
    endif()
    target_sources(cam_node PRIVATE
        src/engines/tensorrt/tensorrt_builder.cpp
        src/engines/tensorrt/tensorrt_engine.cpp
    )
    target_include_directories(cam_node PRIVATE ${TENSORRT_INCLUDE_DIR})
    target_link_libraries(cam_node PRIVATE
        ${TENSORRT_LIBRARY} ${TENSORRT_ONNX_PARSER_LIBRARY} CUDA::cudart)
    target_compile_definitions(cam_node PRIVATE CAM_NODE_WITH_TENSORRT=1)
endif()

option(CAM_NODE_BUILD_TESTS "Build cam_node unit tests" ON)
# Live tests need the configured RealSense camera to run, so they are opt-in.
option(CAM_NODE_LIVE_TESTS "Build cam_node live tests (ctest label: live)" OFF)
if(CAM_NODE_BUILD_TESTS OR CAM_NODE_LIVE_TESTS)
    enable_testing()
endif()
if(CAM_NODE_BUILD_TESTS)
    add_subdirectory(tests/unit)
endif()
if(CAM_NODE_LIVE_TESTS)
    add_subdirectory(tests/live)
endif()
