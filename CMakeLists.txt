cmake_minimum_required(VERSION 3.15)
project(EdgeOS VERSION 0.1.0 LANGUAGES C)

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
add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# ==============================================================================
# 5. Build Core Library (The Pure SDK)
# ==============================================================================
add_library(edge_core_lib STATIC
    src/edge_version.c
    src/shmring.c
    src/datahub.c
    src/arbiter.c
    src/io_executor.c
    src/driver_manager.c
    src/sqlite_writer.c
    src/persistence_mgr.c
    src/algorithm_sum.c
    src/northbound_mgr.c
    src/web_server.c
    src/rest_server.c
    src/services/js_engine.c
    src/services/script_loader.c
    src/core/net_buf.c
    src/core/edge_service.c
    src/driver_modbus.c
    src/modbus_protocol.c
    src/modbus_rtu_driver.c
    src/profile_loader.c
    src/sys_supervisor.c
    src/bus_orchestrator.c
)

target_link_libraries(edge_core_lib PUBLIC 
    cJSON::cjson
    libuv::uv 
    SQLite::SQLite3
    Modbus::Modbus
    QuickJS::QuickJS
    pthread dl rt m
)

target_include_directories(edge_core_lib PUBLIC include include/core include/services)

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
# 7. Tests
# ==============================================================================
enable_testing()
add_subdirectory(tests)
