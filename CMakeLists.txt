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

# [核心补强] 设置嵌入式最佳实践安装前缀
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/edgeos" CACHE PATH "EdgeOS standard install prefix" FORCE)
endif()

set(LOCAL_ROOT "${CMAKE_SOURCE_DIR}/.local/${ARCH}")
message(STATUS "Target Architecture: ${ARCH}")
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
add_definitions(-D_DEFAULT_SOURCE)
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
include(EdgeInstall) # [新增] 引入安装模块
include(BuildContractCompiler)

# 构建编译器二进制 (Go) - 先于产品契约生成
build_contract_compiler()

# 4.2 Copy configuration to build directory for tests
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}/tests")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tests/logs")

# ==============================================================================
# 5. Build Core Library (Decoupled Planes - Layer 1)
# ==============================================================================

# --- Versioning Injection ---
set(EDGE_MAJOR 4)
set(EDGE_MINOR 5)
set(EDGE_PATCH 0)
set(EDGE_VERSION_STR "${EDGE_MAJOR}.${EDGE_MINOR}.${EDGE_PATCH}")

find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE EDGE_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(EDGE_GIT_HASH "unknown")
endif()

string(TIMESTAMP EDGE_BUILD_TIME "%Y-%m-%d %H:%M:%S")

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

target_compile_definitions(edge_core PUBLIC
    EDGE_VER_MAJOR=${EDGE_MAJOR}
    EDGE_VER_MINOR=${EDGE_MINOR}
    EDGE_VER_PATCH=${EDGE_PATCH}
    EDGE_VER_STR="${EDGE_VERSION_STR}"
    EDGE_GIT_HASH="${EDGE_GIT_HASH}"
    EDGE_BUILD_TIME="${EDGE_BUILD_TIME}"
)

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
    src/main_loop/dcu_inventory.c
    src/main_loop/bus_orchestrator.c
    src/main_loop/policy_syncer.c
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
    src/io_executor/driver_dlt645.c
)
target_link_libraries(edge_io_executor PUBLIC edge_core Modbus::Modbus)

# 5.5: External Logic Algorithms (Stateless)
add_library(edge_algo STATIC
    algo/native/algorithm_sum.c
    algo/native/battery_strategy.c
    algo/native/algo_peak_shaving.c
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
add_subdirectory(cmd/device_simulator)

# ==============================================================================
# 8. Installation & Packaging (CPack V3.0)
# ==============================================================================

# 8.1 基础库与公共配置 (Core)
# 使用 USE_SOURCE_PERMISSIONS 保持执行权限，确保软链接不丢失
install(DIRECTORY "${LOCAL_ROOT}/lib/" 
        DESTINATION lib 
        COMPONENT Core
        FILES_MATCHING 
        PATTERN "*.so*"
        PATTERN "*.a" EXCLUDE)

# [核心补强] 开启 RPATH 相对路径搜索
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# 8.4 CPack 全局配置
set(CPACK_PACKAGE_NAME "EdgeOS")
set(CPACK_PACKAGE_VENDOR "EdgeC")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "edgeos-dev@edgec.io")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "EdgeC Engineering Team")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${ARCH}")

set(CPACK_COMPONENTS_ALL Core IEMS DCU EVSE HEMS SG_EL2025)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_GENERATOR "TGZ;DEB")

# DEB 特有配置
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libuv1")

include(CPack)

# ==============================================================================
# 9. Tests
# ==============================================================================
enable_testing()
add_subdirectory(tests)

# ==============================================================================
# 10. Examples
# ==============================================================================
add_subdirectory(examples)
