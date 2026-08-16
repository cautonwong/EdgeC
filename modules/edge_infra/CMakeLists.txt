# ==============================================================================
# EdgeOS Infrastructure & Driver Subsystem (edge_infra)
# ==============================================================================

add_library(edge_infra STATIC
    storage/sqlite_service.c
    storage/sqlite_storage_driver.c
)

target_include_directories(edge_infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)

target_link_libraries(edge_infra PUBLIC edge_common_interface)
target_link_libraries(edge_infra PRIVATE libuv::uv_a SQLite::SQLite3 zlog::zlog)
