add_library(edge_app_host STATIC
    src/app_host_server.c
)

target_include_directories(edge_app_host PUBLIC
    include
)

target_link_libraries(edge_app_host PUBLIC libuv::uv_a)
target_link_libraries(edge_app_host PRIVATE cjson)
