add_library(edge_port_adapters STATIC
    src/edge_port_adapters.c
)

target_include_directories(edge_port_adapters PUBLIC
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_port_adapters PUBLIC edge_core edge_log)
