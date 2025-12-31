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
# 2.1 ZLog Dependency
# ==============================================================================
find_library(ZLOG_LIB NAMES zlog PATHS "${LOCAL_ROOT}/lib" REQUIRED)
include_directories("${LOCAL_ROOT}/include")
message(STATUS "Found zlog: ${ZLOG_LIB}")

# ==============================================================================
# 3. Compiler & Output Options
# ==============================================================================
include(Sanitizers)
include(CodeCoverage)
add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion --coverage)
add_link_options(--coverage)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# Define Global Output Paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sdk/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sdk/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests) # Default for tests

# ==============================================================================
# 4. Build Custom Modules
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EdgeContract)
include(WebBuild)

# 4.1 Build Web UIs (if applicable)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/web/products/iems")
    add_web_product(iems "${CMAKE_CURRENT_SOURCE_DIR}/web/products/iems")
endif()

# 4.2 Copy configuration to build directory for tests
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}/tests")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tests/logs")

# ==============================================================================
# 5. Build Core Library (Decoupled Planes - Layer 1)
# ==============================================================================

# 5.1: The Core Foundation (Shared / Cross-loop)
add_library(edge_core STATIC
    src/core/edge_version.c
    src/core/edge_params.c
    src/core/shmring.c
    src/core/datahub.c
    src/core/net_buf.c
    src/core/sqlite_service.c
    src/core/persistence_mgr.c
    src/core/edge_log.c
)
target_link_libraries(edge_core PUBLIC cJSON::cjson libuv::uv SQLite::SQLite3 ${ZLOG_LIB})

# 5.2: The Control Plane (Main Loop)
add_library(edge_main_loop STATIC
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
)
target_link_libraries(edge_main_loop PUBLIC edge_core PahoMQTT::PahoMQTT)

# 5.3: The Computation Plane (Stream Engine)
add_library(edge_stream_engine STATIC
    src/stream_engine/stream_engine.c
    src/stream_engine/js_engine.c
    src/stream_engine/script_loader.c
    src/stream_engine/script_executor.c
    src/stream_engine/aggregator.c
    src/stream_engine/standard_stages.c
    src/stream_engine/js_stage.c
    src/stream_engine/pipeline_loader.c
    src/stream_engine/stream_dump.c
)
# PUBLIC dependency on main_loop to resolve symbols like service_get_loop and arbiter_submit_cmd
target_link_libraries(edge_stream_engine PUBLIC edge_main_loop QuickJS::QuickJS)

# 5.4: The Execution Plane (IO Executor)
add_library(edge_io_executor STATIC
    src/io_executor/io_executor.c
    src/io_executor/driver_manager.c
    src/io_executor/modbus_rtu_driver.c
    src/io_executor/driver_dlms.c
    src/io_executor/driver_iec104.c
    src/io_executor/driver_can.c
    src/io_executor/profile_loader.c
)
target_link_libraries(edge_io_executor PUBLIC edge_core Modbus::Modbus)

# 5.5: External Logic Algorithms (Stateless)
add_library(edge_algo STATIC
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
)
target_link_libraries(edge_algo PUBLIC edge_core)

# Unified Interface include paths
foreach(lib edge_core edge_main_loop edge_stream_engine edge_io_executor edge_algo)
    target_include_directories(${lib} PUBLIC include algo/include)
endforeach()

# ==============================================================================
# 6. Build Product Applications (Layer 2 - Decentralized)
# ==============================================================================

add_subdirectory(cmd/iems)
add_subdirectory(cmd/sg)
add_subdirectory(cmd/dcu)
add_subdirectory(cmd/evse)
add_subdirectory(cmd/hems)

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
add_subdirectory(examples)
