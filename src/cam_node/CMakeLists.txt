cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(cam_node LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(realsense2 2.58 CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED COMPONENTS core imgproc highgui dnn)

option(TENSORRT "Enable the TensorRT 10+ GPU backend" OFF)

add_executable(cam_node
    src/main.cpp
    src/worker.cpp
    src/camera.cpp
    src/pipeline.cpp
    src/cli/list.cpp
    src/cli/run.cpp
    src/cli/streams.cpp
    src/cli/view.cpp
    src/preview.cpp
    src/stream.cpp
    src/utils.cpp
    src/deprojection.cpp
    src/engines/opencv_dnn_engine.cpp
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

if(TENSORRT)
    find_package(CUDAToolkit REQUIRED)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h
        HINTS ${TensorRT_ROOT} ENV TensorRT_ROOT PATH_SUFFIXES include)
    find_library(TENSORRT_LIBRARY nvinfer
        HINTS ${TensorRT_ROOT} ENV TensorRT_ROOT PATH_SUFFIXES lib lib64)
    if(NOT TENSORRT_INCLUDE_DIR OR NOT TENSORRT_LIBRARY)
        message(FATAL_ERROR "TensorRT headers and nvinfer are required; set TensorRT_ROOT")
    endif()
    target_sources(cam_node PRIVATE src/engines/tensor_rt_engine.cpp)
    target_include_directories(cam_node PRIVATE ${TENSORRT_INCLUDE_DIR})
    target_link_libraries(cam_node PRIVATE ${TENSORRT_LIBRARY} CUDA::cudart)
    target_compile_definitions(cam_node PRIVATE CAM_NODE_WITH_TENSORRT=1)
endif()
