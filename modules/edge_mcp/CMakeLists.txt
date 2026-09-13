cmake_minimum_required(VERSION 3.16)
project(edge_mcp C)

set(EDGE_MCP_SOURCES
    src/edge_mcp_dispatcher.c
    src/transport/mcp_stdio.c
    src/transport/mcp_ws.c
    src/transport/mcp_uds.c
    src/tools/mcp_tools_power_quality.c
    src/tools/mcp_tools_transformer.c
    src/tools/mcp_tools_tou.c
    src/tools/mcp_tools_interlock.c
    src/tools/mcp_tools_registration.c
)

add_library(edge_mcp STATIC ${EDGE_MCP_SOURCES})

target_include_directories(edge_mcp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(edge_mcp PUBLIC
    edge_core
    edge_io
    edge_stream
    edge_infra
    cjson
    libuv::uv_a
    pthread dl rt m
)
