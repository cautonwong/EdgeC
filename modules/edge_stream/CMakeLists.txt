# modules/edge_stream/CMakeLists.txt

file(GLOB STAGE_SOURCES src/stage_*.c)

add_library(edge_stream STATIC
    src/stream_engine.c
    src/pipeline_loader.c
    src/stream_dump.c
    src/js_stage.c
    src/js_engine.c
    src/stage_wasm.c
    src/wasm_runtime/wasm_mgr.c
    src/standard_stages.c
    src/dsp_stages.c
    src/aggregator.c
    src/energy_integrator.c
    src/script_loader.c
    ${STAGE_SOURCES}
)

target_include_directories(edge_stream PUBLIC include)
target_include_directories(edge_stream PRIVATE src/wasm_runtime)
target_link_libraries(edge_stream PUBLIC edge_core edge_algo)
target_link_libraries(edge_stream PRIVATE QuickJS::QuickJS iwasm::vmlib)
