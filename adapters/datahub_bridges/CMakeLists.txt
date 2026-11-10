add_library(edge_datahub_bridges STATIC
    src/stream_bus_datahub_adapter.c
    src/protocol_bridge_datahub_adapter.c
    src/north_source_datahub_adapter.c
    src/radar_datahub_adapter.c
    src/adc_datahub_adapter.c
    src/js_arbiter_service_adapter.c
    src/edge_mcp_datahub_adapter.c
)

target_include_directories(edge_datahub_bridges PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_north/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_datahub_bridges PUBLIC
    edge_core
    edge_stream
    edge_io
    edge_mcp
    edge_north
    edge_log
    edge_common_interface
)
