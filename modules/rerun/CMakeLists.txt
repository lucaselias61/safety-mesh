cmake_minimum_required(VERSION 3.20)

# Shared Rerun connection for cam_node and manager.
# Add this directory once, link rerun_logger.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(rerun_logger LANGUAGES CXX)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Rerun.cmake) # defines rerun_sdk

add_library(rerun_logger STATIC src/viewer_connection.cpp)
target_compile_features(rerun_logger PUBLIC cxx_std_17)
set_target_properties(rerun_logger PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(rerun_logger PUBLIC include)
target_link_libraries(rerun_logger PUBLIC rerun_sdk)
