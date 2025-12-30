cmake_minimum_required(VERSION 3.15)
project(EdgeOS VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# 1. Architecture & Dependency Path
# ==============================================================================
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(TARGET_ARCH "linux-aarch64")
else()
    set(TARGET_ARCH "linux-x86_64")
endif()

set(LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.local/${TARGET_ARCH}")
message(STATUS "Target Architecture: ${TARGET_ARCH}")
message(STATUS "Local Dependency Root: ${LOCAL_ROOT}")

list(APPEND CMAKE_PREFIX_PATH "${LOCAL_ROOT}")
include_directories("${LOCAL_ROOT}/include")
link_directories("${LOCAL_ROOT}/lib")

# ==============================================================================
# 2. Dependency Discovery
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(cJSON REQUIRED)
if(TARGET cjson AND NOT TARGET cJSON::cjson)
    add_library(cJSON::cjson ALIAS cjson)
endif()

find_package(libuv REQUIRED)
if(NOT TARGET libuv::uv)
    if(TARGET uv_a)
        add_library(libuv::uv ALIAS uv_a)
    elseif(TARGET uv)
        add_library(libuv::uv ALIAS uv)
    endif()
endif()

find_package(cmocka REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Modbus REQUIRED)
find_package(QuickJS REQUIRED)
find_package(CURL REQUIRED)
find_package(PahoMQTT REQUIRED)

# ==============================================================================
# 3. Compiler Options
# ==============================================================================
include(Sanitizers)
include(CodeCoverage)
add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion --coverage)
add_link_options(--coverage)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# ==============================================================================
# 5. Build Core Library (The Pure SDK)
# ==============================================================================
# Add a custom step to generate C code from the YAML contracts
find_package(Python3 REQUIRED)
set(PRODUCT_CONTRACT_IEMS ${CMAKE_CURRENT_SOURCE_DIR}/cmd/iems/contract.yaml)
set(GENERATED_DIR_IEMS ${CMAKE_BINARY_DIR}/generated/iems)
set(GENERATED_INCLUDE_DIR_IEMS ${GENERATED_DIR_IEMS}/include)
set(GENERATED_SRC_DIR_IEMS ${GENERATED_DIR_IEMS}/src)
set(GENERATED_TAG_META_SRC_IEMS ${GENERATED_SRC_DIR_IEMS}/tag_meta.c)
set(GENERATED_MAP_MODBUS_SRC_IEMS ${GENERATED_SRC_DIR_IEMS}/map_modbus.c)

add_custom_command(
    OUTPUT ${GENERATED_TAG_META_SRC_IEMS} ${GENERATED_MAP_MODBUS_SRC_IEMS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/contract_compiler.py ${PRODUCT_CONTRACT_IEMS} ${GENERATED_DIR_IEMS}
    DEPENDS ${PRODUCT_CONTRACT_IEMS} ${CMAKE_CURRENT_SOURCE_DIR}/contracts/schema/tags.yaml ${CMAKE_CURRENT_SOURCE_DIR}/contracts/bindings/modbus_maps/default_device.yaml ${CMAKE_CURRENT_SOURCE_DIR}/tools/contract_compiler.py
    COMMENT "Generating contracts for iems"
)

add_custom_target(generate_contracts ALL
    DEPENDS ${GENERATED_TAG_META_SRC_IEMS} ${GENERATED_MAP_MODBUS_SRC_IEMS}
)

# Add the generated include directory to the global path
include_directories(${GENERATED_INCLUDE_DIR_IEMS})

add_library(edge_core_lib STATIC
    # Generated Contract Code
    ${GENERATED_TAG_META_SRC_IEMS}
    ${GENERATED_MAP_MODBUS_SRC_IEMS}

    # Core / Shared Components (Cross-loop)
    src/core/edge_version.c
    src/core/edge_params.c
    src/core/shmring.c
    src/core/datahub.c
    src/core/net_buf.c
    src/core/sqlite_service.c
    src/core/persistence_mgr.c

    # Main Loop (Control Plane)
    src/main_loop/arbiter.c
    src/main_loop/edge_service.c
    src/main_loop/northbound_mgr.c
    src/main_loop/web_server.c
    src/main_loop/rest_server.c
    src/main_loop/ws_proto.c
    src/main_loop/sched_time_source.c
    src/main_loop/sched_calendar.c
    src/main_loop/sched_executor.c
    src/main_loop/sched_libev.c
    src/main_loop/bus_orchestrator.c

    # Stream Engine (Computation Plane)
    src/stream_engine/stream_engine.c
    src/stream_engine/js_engine.c
    src/stream_engine/script_loader.c
    src/stream_engine/script_executor.c
    src/stream_engine/aggregator.c
    src/stream_engine/standard_stages.c
    src/stream_engine/js_stage.c
    src/stream_engine/pipeline_loader.c

    # IO Executor (Execution Plane)
    src/io_executor/io_executor.c
    src/io_executor/driver_manager.c
    src/io_executor/driver_modbus.c
    src/io_executor/modbus_rtu_driver.c
    src/io_executor/profile_loader.c

    # External Logic Algorithms
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
)

target_link_libraries(edge_core_lib PUBLIC
    cJSON::cjson
    libuv::uv
    SQLite::SQLite3
    Modbus::Modbus
    QuickJS::QuickJS
    pthread dl rt m
)

# Make sure contracts are generated before the library is built
add_dependencies(edge_core_lib generate_contracts)

target_include_directories(edge_core_lib PUBLIC include algo/include)

# ==============================================================================
# 6. Build Product Applications (From cmd/)
# ==============================================================================
macro(add_product NAME SRC_DIR)
    add_executable(${NAME} ${SRC_DIR}/main.c)
    target_link_libraries(${NAME} PRIVATE edge_core_lib)
endmacro()

add_product(hems cmd/hems)
add_product(iems cmd/iems)
add_product(sg_el2025 cmd/sg)
add_product(evse cmd/evse)
add_product(dcu cmd/dcu)

# ==============================================================================
# 8. Installation
# ==============================================================================
install(TARGETS edge_core_lib DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/edgeos FILES_MATCHING PATTERN "*.h")

# ==============================================================================
# 9. Tests
# ==============================================================================
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# 10. Examples
# ==============================================================================
add_subdirectory(examples)
