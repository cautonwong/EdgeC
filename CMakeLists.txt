cmake_minimum_required(VERSION 3.15)
project(EdgeOS VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# 0. Build Acceleration (ccache)
# ==============================================================================
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "ccache enabled for C compiler")
endif()

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

# ==============================================================================
# 2. Dependency Discovery
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Clang-Tidy Integration
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY_EXE})
    message(STATUS "Clang-Tidy enabled")
endif()

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
if(NOT TARGET cmocka::cmocka)
    add_library(cmocka::cmocka UNKNOWN IMPORTED)
    set_target_properties(cmocka::cmocka PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LOCAL_ROOT}/include"
        IMPORTED_LOCATION "${LOCAL_ROOT}/lib/libcmocka.a"
    )
endif()

find_package(SQLite3 REQUIRED)
find_package(Modbus REQUIRED)
find_package(QuickJS REQUIRED)
find_package(CURL REQUIRED)
find_package(PahoMQTT REQUIRED)
find_library(ZLOG_LIB NAMES zlog PATHS "${LOCAL_ROOT}/lib" REQUIRED)
add_library(zlog::zlog UNKNOWN IMPORTED)
set_target_properties(zlog::zlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LOCAL_ROOT}/include"
    IMPORTED_LOCATION "${ZLOG_LIB}"
)

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

file(COPY "${CMAKE_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")

# ==============================================================================
# 5. Build Core Library (Decoupled Planes)
# ==============================================================================

add_subdirectory(src/protocols)
add_subdirectory(src/platform)

set(EDGE_MAJOR 4)
set(EDGE_MINOR 5)
set(EDGE_PATCH 0)
set(EDGE_VERSION_STR "${EDGE_MAJOR}.${EDGE_MINOR}.${EDGE_PATCH}")

# 5.1 Base Interface for common includes
add_library(edge_common_interface INTERFACE)
target_include_directories(edge_common_interface INTERFACE 
    include 
    algo/include
    SYSTEM "${LOCAL_ROOT}/include"
)
target_compile_definitions(edge_common_interface INTERFACE _DEFAULT_SOURCE)

# 使用 OBJECT 库来物理消除循环依赖
add_library(edge_core_obj OBJECT
    src/core/edge_version.c
    src/core/edge_params.c
    src/core/shmring.c
    src/core/datahub.c
    src/core/net_buf.c
    src/core/sqlite_service.c
    src/core/persistence_mgr.c
    src/core/edge_log.c
)
target_link_libraries(edge_core_obj PRIVATE edge_common_interface cJSON::cjson libuv::uv SQLite::SQLite3 zlog::zlog)

add_library(edge_main_loop_obj OBJECT
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
target_link_libraries(edge_main_loop_obj PRIVATE edge_common_interface PahoMQTT::PahoMQTT)

add_library(edge_stream_engine_obj OBJECT
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
target_link_libraries(edge_stream_engine_obj PRIVATE edge_common_interface QuickJS::QuickJS)

add_library(edge_io_executor_obj OBJECT
    src/io_executor/driver_manager.c
    src/io_executor/profile_loader.c
    src/io_executor/modbus_rtu_driver.c
    src/io_executor/driver_dlt645.c
    src/io_executor/driver_dlms.c
    src/io_executor/engines/master_engine.c
    src/io_executor/engines/slave_engine.c
)
target_link_libraries(edge_io_executor_obj PRIVATE edge_common_interface Modbus::Modbus)

add_library(edge_algo_obj OBJECT
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
    algo/native/algo_peak_shaving.c
)
target_link_libraries(edge_algo_obj PRIVATE edge_common_interface)

# 定义最终的组合静态库，用于分发和链接
add_library(edge_sdk STATIC
    $<TARGET_OBJECTS:edge_core_obj>
    $<TARGET_OBJECTS:edge_main_loop_obj>
    $<TARGET_OBJECTS:edge_stream_engine_obj>
    $<TARGET_OBJECTS:edge_io_executor_obj>
    $<TARGET_OBJECTS:edge_algo_obj>
    $<TARGET_OBJECTS:edge_protocols_obj>
    $<TARGET_OBJECTS:edge_platform_obj>
)
# 传播依赖项
target_link_libraries(edge_sdk PUBLIC 
    edge_common_interface
    cJSON::cjson 
    libuv::uv 
    SQLite::SQLite3 
    zlog::zlog
    PahoMQTT::PahoMQTT
    QuickJS::QuickJS
    Modbus::Modbus
)

# 为了向后兼容旧的 target 名称，定义 ALIAS
add_library(edge_core ALIAS edge_sdk)
add_library(edge_main_loop ALIAS edge_sdk)
add_library(edge_stream_engine ALIAS edge_sdk)
add_library(edge_io_executor ALIAS edge_sdk)
add_library(edge_algo ALIAS edge_sdk)


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