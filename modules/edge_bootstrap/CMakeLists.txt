# modules/edge_bootstrap/CMakeLists.txt
# EdgeOS Single Composition Root & Application Bootstrap Layer

add_library(edge_bootstrap STATIC
    src/edge_composition_root.c
    src/stream_bus_datahub_adapter.c
    src/protocol_bridge_datahub_adapter.c
    src/radar_datahub_adapter.c
    src/adc_datahub_adapter.c
    src/js_arbiter_service_adapter.c
    src/stream_sqlite_adapter.c
    src/app_repo_sqlite.c
    src/edge_mcp_datahub_adapter.c
    src/edge_port_adapters.c
    src/northbound_driver_adapter.c
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
)

target_link_libraries(edge_bootstrap PUBLIC
    edge_common_interface
    edge_core
    edge_infra
    edge_stream
    edge_north
    edge_io_drivers
)
