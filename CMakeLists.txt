cmake_minimum_required(VERSION 3.20)

include(cmake/version.cmake)
project(safety_mesh VERSION ${SAFETY_MESH_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(safety_mesh_version INTERFACE)
target_include_directories(safety_mesh_version
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_subdirectory(src/cam_node)

