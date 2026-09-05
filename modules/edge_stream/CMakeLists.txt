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
    src/stream_sql_dag.c
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
        src/wasm_runtime/wasm_sandbox_watchdog.c
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

# 自动编译 WASM AI 算子沙箱产物 (如 clang 可用)
find_program(CLANG_PROGRAM clang)
if(CLANG_PROGRAM)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/rehab_companion.wasm"
        COMMAND ${CLANG_PROGRAM} --target=wasm32 -O3 -nostdlib -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -o "${CMAKE_BINARY_DIR}/rehab_companion.wasm" "${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_runtime/rehab_companion.c"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_runtime/rehab_companion.c"
        COMMENT "[Wasm] Compiling Wasm AI Operator rehab_companion.wasm"
    )
    add_custom_target(wasm_rehab_companion ALL DEPENDS "${CMAKE_BINARY_DIR}/rehab_companion.wasm")
    add_dependencies(edge_stream wasm_rehab_companion)
endif()
