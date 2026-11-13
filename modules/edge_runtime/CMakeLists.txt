add_library(edge_runtime STATIC
    src/algorithm_sum.c
    src/edge_cep_engine.c
    src/edge_operator_fusion.c
    src/edge_wasm_nn.c
    src/federated_aggregator.c
    src/onnx_host_guard.c
)

target_include_directories(edge_runtime PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_link_libraries(edge_runtime PUBLIC edge_math edge_foundation libuv::uv_a)
