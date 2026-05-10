# Zephyr Application CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ${ZEPHYR_BASE})
project(edge_node_zephyr)

# 修正：源文件就在当前目录
target_sources(app PRIVATE 
    main.c
    uv_rtos.c
    edge_osal_zephyr.c
)

target_include_directories(app PRIVATE 
    .
    ../../include
)

target_compile_definitions(app PRIVATE _POSIX_C_SOURCE=200809L)
