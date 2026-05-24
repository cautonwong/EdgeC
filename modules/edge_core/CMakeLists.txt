# modules/edge_core/CMakeLists.txt

add_library(edge_core STATIC
    src/edge_version.c
    src/shmring.c
    src/datahub.c
    src/net_buf.c
    src/edge_log.c
    src/contract_registry.c
    src/arbiter.c
    src/edge_params.c
    src/edge_os_zephyr.c
    src/sqlite_service.c
    src/edge_service.c
)

# PUBLIC: 链接 edge_core 的 Target 也能 include 这些头文件
target_include_directories(edge_core PUBLIC
    include
)

# PRIVATE: 依赖项
target_link_libraries(edge_core PUBLIC edge_common_interface)
target_link_libraries(edge_core PRIVATE libuv::uv_a cjson zlog::zlog SQLite::SQLite3)
