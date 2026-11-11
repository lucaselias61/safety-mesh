cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(manager LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Standalone builds pull in the shared comm module directly.
if(NOT TARGET comm_protobuf)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../comm" "${CMAKE_CURRENT_BINARY_DIR}/comm")
endif()

# Robot PLC input, shared by the manager, tools and tests.
add_library(manager_robot STATIC
    src/robot_pose.cpp
)
target_compile_features(manager_robot PUBLIC cxx_std_17)
target_include_directories(manager_robot PUBLIC include)
target_link_libraries(manager_robot PUBLIC comm_ads)

add_executable(manager
    src/main.cpp
    src/manager.cpp
)

set(MANAGER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Executable output directory")
set_target_properties(manager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MANAGER_OUTPUT_DIR}")

target_compile_features(manager PRIVATE cxx_std_17)
target_include_directories(manager PRIVATE include)
target_link_libraries(manager PRIVATE
    CLI11::CLI11
    nlohmann_json::nlohmann_json
    fmt::fmt
    comm_protobuf
    manager_robot
)

# Prints the robot pose read from the PLC: bin/ads_pose_probe --symbol ...
add_executable(ads_pose_probe tools/ads_pose_probe.cpp)
set_target_properties(ads_pose_probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MANAGER_OUTPUT_DIR}")
target_link_libraries(ads_pose_probe PRIVATE CLI11::CLI11 fmt::fmt manager_robot)

option(MANAGER_BUILD_TESTS "Build manager tests" ON)
if(MANAGER_BUILD_TESTS)
    enable_testing()
    add_executable(manager_robot_pose_test tests/robot_pose_test.cpp)
    target_link_libraries(manager_robot_pose_test PRIVATE manager_robot)
    add_test(NAME manager_robot_pose COMMAND manager_robot_pose_test)
endif()
