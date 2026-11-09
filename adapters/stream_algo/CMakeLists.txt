add_library(edge_algo_stream STATIC
    src/edge_algo_stream_adapter.c
)

target_include_directories(edge_algo_stream PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_stream/include
    ${CMAKE_SOURCE_DIR}/modules/edge_algo/include
    ${CMAKE_SOURCE_DIR}/foundation/include
)

target_link_libraries(edge_algo_stream PUBLIC edge_algo)
