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

# Define where our vendors are installed (The .local directory which is now gitignored)
set(LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.local/${TARGET_ARCH}")
message(STATUS "Target Architecture: ${TARGET_ARCH}")
message(STATUS "Local Dependency Root: ${LOCAL_ROOT}")

# Tell CMake to look for packages here first
list(APPEND CMAKE_PREFIX_PATH "${LOCAL_ROOT}")

# Also add to include/link paths globally for Find modules that might be simplistic
include_directories("${LOCAL_ROOT}/include")
link_directories("${LOCAL_ROOT}/lib")

# ==============================================================================
# 2. Dependency Discovery (Standard find_package)
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 2.1 cJSON
find_package(cJSON REQUIRED)
if(TARGET cjson AND NOT TARGET cJSON::cjson)
    add_library(cJSON::cjson ALIAS cjson)
endif()

# 2.2 libuv
find_package(libuv REQUIRED)
if(NOT TARGET libuv::uv)
    if(TARGET uv_a)
        add_library(libuv::uv ALIAS uv_a)
    elseif(TARGET uv)
        add_library(libuv::uv ALIAS uv)
    endif()
endif()

# 2.3 cmocka
find_package(cmocka REQUIRED)

# 2.4 SQLite3
find_package(SQLite3 REQUIRED)

# 2.5 libmodbus
find_package(Modbus REQUIRED)

# 2.6 QuickJS
find_package(QuickJS REQUIRED)

# 2.7 CURL
find_package(CURL REQUIRED)

# 2.8 PahoMQTT
find_package(PahoMQTT REQUIRED)

# ==============================================================================
# 3. Compiler Options & Tools
# ==============================================================================
include(Sanitizers)
include(CodeCoverage)

add_compile_options(-Wall -Wextra -Wformat=2 -Wconversion)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# ==============================================================================
# 5. Build Core Library
# ==============================================================================
add_library(edge_core_lib STATIC
    src/edge_version.c
    src/shmring.c
    src/algorithm_sum.c
    src/sqlite_writer.c
    src/sqlite_service.c
    src/persistence_mgr.c
    src/northbound_mgr.c
    src/sys_supervisor.c
    src/core/net_buf.c
    src/arbiter.c
    src/io_executor.c
    src/driver_manager.c
    src/bus_orchestrator.c
    src/driver_modbus.c
    src/modbus_protocol.c
    src/modbus_rtu_driver.c
    src/profile_loader.c
    src/datahub.c
    src/core/edge_service.c
    src/rest_server.c
    src/web_server.c
)

target_link_libraries(edge_core_lib PUBLIC 
    cJSON::cjson
    libuv::uv 
    SQLite::SQLite3
    Modbus::Modbus
    pthread dl rt m
)

target_include_directories(edge_core_lib PUBLIC include)

# ==============================================================================
# 6. Build Product Applications (Cmd)
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