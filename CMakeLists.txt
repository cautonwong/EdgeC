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
find_package(Python3 REQUIRED)

# --- Section 5.2: Define the Core SDK Library ---
# --- Section 5.2: Define the Core SDK Library ---
set(EDGE_CORE_LIB_SOURCES
    # Core / Shared Components (Cross-loop)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/edge_version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/edge_params.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/shmring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/datahub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/net_buf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sqlite_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/persistence_mgr.c

    # Main Loop (Control Plane)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/arbiter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/edge_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/northbound_mgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/web_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/rest_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/ws_proto.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/sched_time_source.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/sched_calendar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/sched_executor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/sched_libev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_loop/bus_orchestrator.c

    # Stream Engine (Computation Plane)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/stream_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/js_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/script_loader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/script_executor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/aggregator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/standard_stages.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/js_stage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream_engine/pipeline_loader.c

    # IO Executor (Execution Plane)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io_executor/io_executor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io_executor/driver_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io_executor/driver_modbus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io_executor/profile_loader.c

    # External Logic Algorithms
    ${CMAKE_CURRENT_SOURCE_DIR}/algo/native/algorithm_sum.c
    ${CMAKE_CURRENT_SOURCE_DIR}/algo/native/battery_strategy.c
)

# ==============================================================================
# 6. Build Product Applications (From cmd/)
# ==============================================================================
function(add_product_with_contracts PRODUCT_NAME)
    # If a second argument is provided, use it as the source subdirectory
    if(ARGC GREATER 1)
        set(SRC_SUBDIR ${ARGV1})
    else()
        set(SRC_SUBDIR ${PRODUCT_NAME})
    endif()

    set(CONTRACT_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/contract_compiler.py)
    set(SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/contracts/schema/tags.yaml)
    set(PRODUCT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/contracts/products/${PRODUCT_NAME}.yaml)
    set(LOCAL_EXT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmd/${SRC_SUBDIR}/extension.yaml)
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated/${PRODUCT_NAME})

    set(GENERATED_C ${OUTPUT_DIR}/contracts.c)
    set(GENERATED_H ${OUTPUT_DIR}/contracts.h)

    # Check if the optional local extension file exists
    set(LOCAL_CMD_ARG "")
    set(LOCAL_DEPENDENCY "")
    if(EXISTS ${LOCAL_EXT_FILE})
        set(LOCAL_CMD_ARG --local ${LOCAL_EXT_FILE})
        set(LOCAL_DEPENDENCY ${LOCAL_EXT_FILE})
        message(STATUS "Found local contract extension for ${PRODUCT_NAME}")
    endif()

    # Add the command to generate contracts for this specific product
    add_custom_command(
        OUTPUT ${GENERATED_C} ${GENERATED_H}
        COMMAND ${Python3_EXECUTABLE} ${CONTRACT_SCRIPT}
            --schema ${SCHEMA_FILE}
            --product ${PRODUCT_FILE}
            ${LOCAL_CMD_ARG}
            --out ${OUTPUT_DIR}
        DEPENDS ${CONTRACT_SCRIPT} ${SCHEMA_FILE} ${PRODUCT_FILE} ${LOCAL_DEPENDENCY}
        COMMENT "Generating contracts for ${PRODUCT_NAME}"
    )

    # Create a library for the generated contract code
    set(CONTRACT_LIB_NAME "contracts_${PRODUCT_NAME}")
    add_library(${CONTRACT_LIB_NAME} STATIC ${GENERATED_C})
    target_include_directories(${CONTRACT_LIB_NAME} PUBLIC ${OUTPUT_DIR})

    # Create the final executable for the product
    add_executable(${PRODUCT_NAME} 
        ${CMAKE_CURRENT_SOURCE_DIR}/cmd/${SRC_SUBDIR}/main.c
        ${EDGE_CORE_LIB_SOURCES}
    )
    target_include_directories(${PRODUCT_NAME} PRIVATE include algo/include)
    target_link_libraries(${PRODUCT_NAME} PRIVATE
        ${CONTRACT_LIB_NAME}
        cJSON::cjson
        libuv::uv
        SQLite::SQLite3
        Modbus::Modbus
        QuickJS::QuickJS
        pthread dl rt m
    )
endfunction()

# Now, add all products using the new function
add_product_with_contracts(iems)
add_product_with_contracts(sg_el2025 sg)

# ==============================================================================
# 8. Installation
# ==============================================================================
install(DIRECTORY include/ DESTINATION include/edgeos FILES_MATCHING PATTERN "*.h")

# ==============================================================================
# 9. Tests
# ==============================================================================
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# 10. Examples
# ==============================================================================
# add_subdirectory(examples)
