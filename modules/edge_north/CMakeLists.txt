# modules/edge_north/CMakeLists.txt

add_library(edge_north STATIC
    src/northbound_mgr.c
    src/snf_manager.c
)

target_include_directories(edge_north PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_north PUBLIC
    edge_codec
    edge_log
    edge_common_interface
    edge_foundation
)
target_link_libraries(edge_north PRIVATE libuv::uv_a cjson CURL::libcurl SQLite::SQLite3)
