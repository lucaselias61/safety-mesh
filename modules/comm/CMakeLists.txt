cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(comm LANGUAGES CXX)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Targets, all in namespace comm:
# - comm_protobuf: message schemas and datagram codec (protobuf).
# - comm_udp: UDP datagrams as raw bytes.
# - comm_ads: reads TwinCAT PLC variables over ADS.

# --- comm_protobuf -----------------------------------------------------------

# Newer protobuf (Homebrew) ships a CMake package config
# Ubuntu 24.04's 3.2 only works through CMake's FindProtobuf module.
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()

add_library(comm_protobuf STATIC
    proto/camera_frame.proto
    src/codec.cpp
)

# Generated headers land in <build>/generated/comm/*.pb.h, included as
# <comm/camera_frame.pb.h>. They are build outputs and never committed.
set(COMM_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${COMM_GENERATED_DIR}/comm")
protobuf_generate(
    TARGET comm_protobuf
    LANGUAGE cpp
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    PROTOC_OUT_DIR "${COMM_GENERATED_DIR}/comm"
)

target_compile_features(comm_protobuf PUBLIC cxx_std_17)
set_target_properties(comm_protobuf PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(comm_protobuf PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${COMM_GENERATED_DIR}"
)
target_link_libraries(comm_protobuf PUBLIC protobuf::libprotobuf)

# --- comm_udp ----------------------------------------------------------------

add_library(comm_udp STATIC
    src/udp.cpp
)
target_compile_features(comm_udp PUBLIC cxx_std_17)
set_target_properties(comm_udp PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(comm_udp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# --- comm_ads ----------------------------------------------------------------

option(COMM_BUILD_ADS "Build the ADS (TwinCAT PLC) client" ON)
if(COMM_BUILD_ADS)
    include(cmake/BeckhoffAds.cmake)  # defines beckhoff_adslib

    add_library(comm_ads STATIC
        src/ads.cpp
    )
    target_compile_features(comm_ads PUBLIC cxx_std_17)
    set_target_properties(comm_ads PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(comm_ads PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    # PRIVATE: the AdsLib headers stay an implementation detail of ads.cpp.
    target_link_libraries(comm_ads PRIVATE beckhoff_adslib)
endif()

# --- tests -------------------------------------------------------------------

option(COMM_BUILD_TESTS "Build comm tests and demos" ON)
if(COMM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
