# modules/edge_north/CMakeLists.txt

add_library(edge_north STATIC
    src/northbound_mgr.c
    src/snf_manager.c
)

target_include_directories(edge_north PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/adapters/io_drivers/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_north PUBLIC
    edge_core
    edge_infra
    edge_codec
    edge_io_drivers
    edge_log
    edge_common_interface
)
target_link_libraries(edge_north PRIVATE libuv::uv_a cjson CURL::libcurl SQLite::SQLite3)
