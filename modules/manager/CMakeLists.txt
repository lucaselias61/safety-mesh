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
)
