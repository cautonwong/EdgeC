add_library(edge_api STATIC
    src/edge_api.c
)

target_include_directories(edge_api PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_io/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_api PUBLIC
    edge_io
    edge_core
    edge_log
)
