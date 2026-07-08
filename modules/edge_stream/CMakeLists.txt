# modules/edge_stream/CMakeLists.txt

file(GLOB STAGE_SOURCES src/stage_*.c)

set(STREAM_SOURCES
    src/stream_engine.c
    src/pipeline_loader.c
    src/stream_dump.c
    src/standard_stages.c
    src/dsp_stages.c
    src/aggregator.c
    src/energy_integrator.c
    src/script_loader.c
    ${STAGE_SOURCES}
)

# 根据 Kconfig 条件加载嵌入式 QuickJS 脚本引擎 (以接地气的中文添加注释)
if(CONFIG_EDGEC_JS_ENGINE)
    list(APPEND STREAM_SOURCES
        src/js_stage.c
        src/js_engine.c
    )
endif()

# 根据 Kconfig 条件加载 WAMR (WebAssembly Micro Runtime) 安全算子运行沙箱
if(CONFIG_EDGEC_WASM)
    list(APPEND STREAM_SOURCES
        src/stage_wasm.c
        src/wasm_runtime/wasm_mgr.c
        src/wasm_runtime/wasm_app_scheduler.c
    )
endif()

add_library(edge_stream STATIC
    ${STREAM_SOURCES}
)

target_include_directories(edge_stream PUBLIC include)
target_include_directories(edge_stream PRIVATE src/wasm_runtime)
target_link_libraries(edge_stream PUBLIC edge_core edge_algo)
find_package(OpenSSL REQUIRED)
target_link_libraries(edge_stream PRIVATE QuickJS::QuickJS iwasm::vmlib OpenSSL::Crypto)
