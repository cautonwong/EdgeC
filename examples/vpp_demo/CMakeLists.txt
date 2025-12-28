cmake_minimum_required(VERSION 3.10)
project(vpp_demo)

if(TARGET edge_core_lib)
    # In-tree build
    include_directories(../../algo/include)
    add_executable(vpp_demo main.c)
    target_link_libraries(vpp_demo PRIVATE edge_core_lib libuv::uv QuickJS::QuickJS)
else()
    # Standalone build
    find_library(EDGEOS_LIB edge_core_lib)
    find_path(EDGEOS_INCLUDE_DIR NAMES edgeos/edge_service.h)

    if (NOT EDGEOS_LIB OR NOT EDGEOS_INCLUDE_DIR)
        message(FATAL_ERROR "EdgeOS SDK not found! Please run 'make install' first.")
    endif()

    find_package(libuv REQUIRED)

    add_executable(vpp_demo main.c)
    target_include_directories(vpp_demo PRIVATE ${EDGEOS_INCLUDE_DIR}/..)
    target_link_libraries(vpp_demo PRIVATE ${EDGEOS_LIB} libuv::uv)
endif()
