cmake_minimum_required(VERSION 3.15)
project(EdgeOS VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# 1. Architecture & Dependency Path
# ==============================================================================
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ARCH "linux-aarch64")
else()
    set(ARCH "linux-x86_64")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/edgeos" CACHE PATH "EdgeOS standard install prefix" FORCE)
endif()

set(LOCAL_ROOT "${CMAKE_SOURCE_DIR}/.local/${ARCH}")
list(APPEND CMAKE_PREFIX_PATH "${LOCAL_ROOT}")
include_directories("${LOCAL_ROOT}/include")
link_directories("${LOCAL_ROOT}/lib")

# ==============================================================================
# 2. Dependency Discovery
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(cJSON REQUIRED)
# Ensure cJSON::cjson target exists
if(NOT TARGET cJSON::cjson)
    if(TARGET cjson)
        add_library(cJSON::cjson ALIAS cjson)
    else()
        add_library(cJSON::cjson UNKNOWN IMPORTED)
        set_target_properties(cJSON::cjson PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CJSON_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${CJSON_LIBRARIES}"
        )
    endif()
endif()

find_package(libuv REQUIRED)
find_package(cmocka REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Modbus REQUIRED)
find_package(QuickJS REQUIRED)
find_package(CURL REQUIRED)
find_package(PahoMQTT REQUIRED)
find_library(ZLOG_LIB NAMES zlog PATHS "${LOCAL_ROOT}/lib" REQUIRED)

# ==============================================================================
# 3. Compiler & Output Options
# ==============================================================================
add_definitions(-D_DEFAULT_SOURCE)
include(Sanitizers)
include(CodeCoverage)
add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion --coverage)
add_link_options(--coverage)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sdk/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sdk/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

# ==============================================================================
# 4. Custom Modules & Submodules
# ==============================================================================
include(EdgeContract)
include(WebBuild)
include(EdgeInstall)
include(BuildContractCompiler)

build_contract_compiler()

# [规约内核层] 引入专家协议库
add_subdirectory(libs/libedge-proto)

file(COPY "${CMAKE_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")

# ==============================================================================
# 5. Build Core Library (Decoupled Planes)
# ==============================================================================

set(EDGE_MAJOR 4)
set(EDGE_MINOR 5)
set(EDGE_PATCH 0)
set(EDGE_VERSION_STR "${EDGE_MAJOR}.${EDGE_MINOR}.${EDGE_PATCH}")

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
    src/main_loop/dcu_inventory.c
    src/main_loop/bus_orchestrator.c
    src/main_loop/policy_syncer.c
)
target_link_libraries(edge_main_loop PUBLIC edge_core PahoMQTT::PahoMQTT)

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
target_link_libraries(edge_stream_engine PUBLIC edge_main_loop QuickJS::QuickJS)

# 5.4: IO Executor (基于专家库重塑)
add_library(edge_io_executor STATIC
    src/io_executor/io_executor.c
    src/io_executor/driver_manager.c
    src/io_executor/modbus_rtu_driver.c
    src/io_executor/driver_dlms.c
    src/io_executor/driver_iec104.c
    src/io_executor/driver_can.c
    src/io_executor/profile_loader.c
    src/io_executor/driver_dlt645.c
)
# 链接 edge_proto 规约内核
target_link_libraries(edge_io_executor PUBLIC edge_core edge_proto Modbus::Modbus)

add_library(edge_algo STATIC
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
    algo/native/algo_peak_shaving.c
)
target_link_libraries(edge_algo PUBLIC edge_core)

foreach(lib edge_core edge_main_loop edge_stream_engine edge_io_executor edge_algo)
    target_include_directories(${lib} PUBLIC include algo/include)
endforeach()

# ==============================================================================
# 6. Build Product Applications
# ==============================================================================
add_subdirectory(cmd/iems)
add_subdirectory(cmd/sg)
add_subdirectory(cmd/dcu)
add_subdirectory(cmd/evse)
add_subdirectory(cmd/hems)
add_subdirectory(cmd/device_simulator)

include(CPack)
enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)