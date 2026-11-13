add_library(edge_rest_api STATIC
    src/rest_server.c
    src/rest_apps_package.c
    src/rest_uds_server.c
    src/web_server.c
)

target_include_directories(edge_rest_api PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_rest_api PUBLIC
    edge_core
    edge_io
    edge_api
    edge_log
)
target_link_libraries(edge_rest_api PRIVATE libuv::uv_a cjson)
