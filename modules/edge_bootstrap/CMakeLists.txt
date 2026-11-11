# modules/edge_bootstrap/CMakeLists.txt
# EdgeOS Single Composition Root & Application Bootstrap Layer

add_library(edge_bootstrap STATIC
    src/edge_composition_root.c
    src/edge_driver_runtime.c
)

target_include_directories(edge_bootstrap PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_mcp/include
    ${CMAKE_SOURCE_DIR}/modules/edge_mgmt/include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/adapters/io_drivers/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_bootstrap PUBLIC
    edge_common_interface
    edge_datahub_bridges
    edge_sqlite_adapters
    edge_port_adapters
    edge_core
    edge_infra
    edge_stream
    edge_north
    edge_infra_security
    edge_io_drivers
)
