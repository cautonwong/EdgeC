# third-party/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(LOCAL_CYCLONE_LIB "${LOCAL_ROOT}/lib/libddsc.so")
set(LOCAL_CYCLONE_INC "${LOCAL_ROOT}/include")

if(EXISTS "${LOCAL_CYCLONE_LIB}")
    add_library(cyclonedds UNKNOWN IMPORTED GLOBAL)
    set_target_properties(cyclonedds PROPERTIES
        IMPORTED_LOCATION "${LOCAL_CYCLONE_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LOCAL_CYCLONE_INC}"
    )
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/src/cyclonedds.c")
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
else()
    add_library(cyclonedds INTERFACE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/include")
        target_include_directories(cyclonedds INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/cyclonedds/include
        )
    endif()
endif()

if(NOT TARGET ddsc)
    add_library(ddsc ALIAS cyclonedds)
endif()

set(_edgec_c_flags_saved "${CMAKE_C_FLAGS}")
set(UA_ENABLE_AMALGAMATION OFF CACHE BOOL "" FORCE)
set(UA_ENABLE_HISTORIZING OFF CACHE BOOL "" FORCE)
set(UA_ENABLE_PUBSUB OFF CACHE BOOL "" FORCE)
set(UA_ENABLE_ENCRYPTION OFF CACHE STRING "" FORCE)
set(UA_NAMESPACE_ZERO REDUCED CACHE STRING "" FORCE)
set(UA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(UA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(UA_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
add_subdirectory(open62541)
set(CMAKE_C_FLAGS "${_edgec_c_flags_saved}" CACHE STRING "" FORCE)
target_compile_options(open62541-object PRIVATE -fno-strict-aliasing)
target_compile_options(open62541-plugins PRIVATE -fno-strict-aliasing)
