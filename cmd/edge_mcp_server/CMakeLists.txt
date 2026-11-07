cmake_minimum_required(VERSION 3.16)
project(edge_mcp_server C)

add_executable(edge-mcp-server main.c)

target_include_directories(edge-mcp-server PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_bootstrap/include
)

target_link_libraries(edge-mcp-server PRIVATE
    edge_mcp
    edge_core
    edge_io
    edge_stream
    edge_infra
    edge_bootstrap
    cjson
    libuv::uv_a
    pthread dl rt m
)

install(TARGETS edge-mcp-server DESTINATION bin)
