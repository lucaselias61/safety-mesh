set(SAFETY_MESH_VERSION_HEADER "${CMAKE_CURRENT_LIST_DIR}/../include/safety_mesh/version.hpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SAFETY_MESH_VERSION_HEADER}")

foreach(component MAJOR MINOR PATCH)
    file(STRINGS "${SAFETY_MESH_VERSION_HEADER}" definition
        REGEX "^#define SAFETY_MESH_VERSION_${component} [0-9]+$")
    if(NOT definition MATCHES "^#define SAFETY_MESH_VERSION_${component} ([0-9]+)$")
        message(FATAL_ERROR "Invalid ${component} version in ${SAFETY_MESH_VERSION_HEADER}")
    endif()
    set(SAFETY_MESH_VERSION_${component} "${CMAKE_MATCH_1}")
endforeach()

set(SAFETY_MESH_VERSION "${SAFETY_MESH_VERSION_MAJOR}.${SAFETY_MESH_VERSION_MINOR}.${SAFETY_MESH_VERSION_PATCH}")
