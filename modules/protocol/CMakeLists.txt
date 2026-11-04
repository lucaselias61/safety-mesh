cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(protocol LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Newer protobuf (Homebrew) ships a CMake package config; Ubuntu 24.04's 3.21
# only works through CMake's FindProtobuf module.
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()

add_library(protocol STATIC
    proto/camera_frame.proto
    src/codec.cpp
)

# Generated headers land in <build>/generated/protocol/*.pb.h, included as
# <protocol/camera_frame.pb.h>. They are build outputs and never committed.
set(PROTOCOL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTOCOL_GENERATED_DIR}/protocol")
protobuf_generate(
    TARGET protocol
    LANGUAGE cpp
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${PROTOCOL_GENERATED_DIR}/protocol"
)

target_compile_features(protocol PUBLIC cxx_std_17)
set_target_properties(protocol PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(protocol PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROTOCOL_GENERATED_DIR}"
)
target_link_libraries(protocol PUBLIC protobuf::libprotobuf)

option(PROTOCOL_BUILD_TESTS "Build protocol tests and demos" ON)
if(PROTOCOL_BUILD_TESTS)
    add_subdirectory(tests)
endif()
