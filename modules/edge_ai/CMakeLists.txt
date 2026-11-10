add_library(edge_ai STATIC
    src/ai_awq_gptq_gemm.c
    src/ai_dynamic_tensor_pool.c
    src/ai_fast_gemm.c
    src/ai_graph_pruner.c
    src/ai_hetero_pipeline.c
    src/ai_pinn_evaluator.c
    src/ai_quant_calibrator.c
    src/ai_slm_kv_cache.c
    src/ai_temporal_fault_net.c
)

target_include_directories(edge_ai PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/foundation/include
)
target_link_libraries(edge_ai PUBLIC edge_math)
