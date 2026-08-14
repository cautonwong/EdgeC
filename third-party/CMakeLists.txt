# third-party/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

add_library(cyclonedds STATIC
    cyclonedds/src/cyclonedds.c
)

target_include_directories(cyclonedds PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/src/core/ddsc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/src/ddsrt/include
)

set_target_properties(cyclonedds PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

if(NOT TARGET ddsc)
    add_library(ddsc ALIAS cyclonedds)
endif()
