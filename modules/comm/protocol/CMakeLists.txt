cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(comm_protocol LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Newer protobuf (Homebrew) ships a CMake package config; Ubuntu 24.04's 3.21
# only works through CMake's FindProtobuf module.
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()

add_library(comm_protocol STATIC
    proto/camera_frame.proto
    src/codec.cpp
)

# Generated headers land in <build>/generated/comm/protocol/*.pb.h, included as
# <comm/protocol/camera_frame.pb.h>. They are build outputs and never committed.
set(PROTOCOL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTOCOL_GENERATED_DIR}/comm/protocol")
protobuf_generate(
    TARGET comm_protocol
    LANGUAGE cpp
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${PROTOCOL_GENERATED_DIR}/comm/protocol"
)

target_compile_features(comm_protocol PUBLIC cxx_std_17)
set_target_properties(comm_protocol PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(comm_protocol PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROTOCOL_GENERATED_DIR}"
)
target_link_libraries(comm_protocol PUBLIC protobuf::libprotobuf)

option(PROTOCOL_BUILD_TESTS "Build protocol tests and demos" ON)
if(PROTOCOL_BUILD_TESTS)
    add_subdirectory(tests)
endif()
