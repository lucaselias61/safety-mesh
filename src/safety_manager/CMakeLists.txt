cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(safety_manager LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(safety_manager
    src/main.cpp
    src/manager.cpp
)

set(SAFETY_MANAGER_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Executable output directory")
set_target_properties(safety_manager PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${SAFETY_MANAGER_OUTPUT_DIR}")

target_compile_features(safety_manager PRIVATE cxx_std_17)
target_include_directories(safety_manager PRIVATE include)
target_link_libraries(safety_manager PRIVATE
    CLI11::CLI11
    nlohmann_json::nlohmann_json
    fmt::fmt
)
