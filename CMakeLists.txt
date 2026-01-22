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

# 移除全局选项，改为在 edge_common_interface 中传播
# add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion --coverage)
# add_link_options(--coverage)

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
add_subdirectory(src/ui)

set(EDGE_MAJOR 4)
set(EDGE_MINOR 5)
set(EDGE_PATCH 0)
set(EDGE_VERSION_STR "${EDGE_MAJOR}.${EDGE_MINOR}.${EDGE_PATCH}")

# 5.1 Base Interface for common includes and flags
add_library(edge_common_interface INTERFACE)
target_include_directories(edge_common_interface INTERFACE 
    include 
    algo/include
    SYSTEM "${LOCAL_ROOT}/include"
)
target_compile_definitions(edge_common_interface INTERFACE _DEFAULT_SOURCE)
# 属性传播：将编译和链接选项封装在接口中
target_compile_options(edge_common_interface INTERFACE 
    -Wall -Wextra -Wformat=2 -Wconversion --coverage
)
target_link_options(edge_common_interface INTERFACE --coverage)

# 5.2 核心平面 (Foundation L1 - 原子库)
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
target_link_libraries(edge_core PUBLIC edge_common_interface)
target_link_libraries(edge_core PRIVATE cJSON::cjson libuv::uv SQLite::SQLite3 zlog::zlog)

# 5.3 计算平面 (Compute Plane)
add_library(edge_stream_engine STATIC
    src/stream_engine/stream_engine.c
    src/stream_engine/js_engine.c
    src/stream_engine/script_loader.c
    src/stream_engine/aggregator.c
    src/stream_engine/standard_stages.c
    src/stream_engine/js_stage.c
    src/stream_engine/pipeline_loader.c
    src/stream_engine/stream_dump.c
)
# 注意：由于代码中存在交叉引用，我们需要在链接时允许循环
target_link_libraries(edge_stream_engine PUBLIC edge_core edge_algo)
target_link_libraries(edge_stream_engine PRIVATE QuickJS::QuickJS)

# 5.4 执行平面 (Execution Plane)
add_library(edge_io_executor STATIC
    src/io_executor/driver_manager.c
    src/io_executor/profile_loader.c
    src/io_executor/modbus_rtu_driver.c
    src/io_executor/driver_dlt645.c
    src/io_executor/driver_dlms.c
    src/io_executor/driver_iec104.c
    src/io_executor/driver_iec101.c
    src/io_executor/engines/master_engine.c
    src/io_executor/engines/slave_engine.c
    src/io_executor/requests/req_driver_legacy.c
    src/io_executor/requests/req_raw_io.c
    src/io_executor/edge_api.c
)
target_include_directories(edge_io_executor PUBLIC
    src/io_executor/requests
)
target_link_libraries(edge_io_executor PUBLIC edge_core edge_protocols edge_platform)
target_link_libraries(edge_io_executor PRIVATE Modbus::Modbus)

# 5.5 控制平面 (Control Plane)
add_library(edge_main_loop STATIC
    src/main_loop/arbiter.c
    src/main_loop/bus_orchestrator.c
    src/main_loop/dcu_inventory.c
    src/main_loop/edge_service.c
    src/main_loop/northbound_mgr.c
    src/main_loop/policy_syncer.c
    src/main_loop/rest_server.c
    src/main_loop/sched_engine.c
    src/main_loop/sched_port.c
    src/main_loop/web_server.c
    src/main_loop/ws_proto.c
)
# main_loop 依赖 io_executor 和 stream_engine
target_link_libraries(edge_main_loop PUBLIC edge_core edge_stream_engine edge_io_executor)
target_link_libraries(edge_main_loop PRIVATE PahoMQTT::PahoMQTT)

# 解决循环依赖：让 stream_engine 也能看到 main_loop 的符号
target_link_libraries(edge_stream_engine PUBLIC edge_main_loop)

# 5.6 算法库
add_library(edge_algo STATIC
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
    algo/native/algo_peak_shaving.c
)
target_link_libraries(edge_algo PUBLIC edge_common_interface)

# 定义最终的组合静态库 (接口库)
add_library(edge_sdk INTERFACE)
target_link_libraries(edge_sdk INTERFACE 
    edge_main_loop 
)

# ==============================================================================
# 6. Build Product Applications
# ==============================================================================
add_subdirectory(cmd)

include(CPack)
enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)