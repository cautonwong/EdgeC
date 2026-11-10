add_library(edge_sqlite_adapters STATIC
    src/app_repo_sqlite.c
    src/stream_sqlite_adapter.c
    src/snf_sqlite_adapter.c
    src/edge_params_sqlite_adapter.c
)

target_include_directories(edge_sqlite_adapters PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

if(CONFIG_EDGEC_PERSISTENCE)
    target_sources(edge_sqlite_adapters PRIVATE src/persistence_mgr.c)
endif()

target_link_libraries(edge_sqlite_adapters PUBLIC
    edge_core
    edge_infra
    edge_stream
    edge_mgmt
    edge_north
    edge_log
    edge_common_interface
)
target_link_libraries(edge_sqlite_adapters PRIVATE SQLite::SQLite3)
