cmake_minimum_required(VERSION 3.15)
project(EdgeOS_Seed VERSION 0.1.0 LANGUAGES C)

# ==============================================================================
# 1. Global Configuration
# ==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)
include(CodeCoverage)

# ==============================================================================
# 2. Options & Compiler Flags
# ==============================================================================
option(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)

add_compile_options(
    -Wall -Wextra -Wformat=2 -Wconversion
    # -Werror # Stricter in production
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
    else()
        message(WARNING "clang-tidy not found")
    endif()
endif()

# ==============================================================================
# 3. Dependency Injection (Hermetic Build)
# ==============================================================================
# Detect Arch
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(TARGET_ARCH "linux-aarch64")
else()
    set(TARGET_ARCH "linux-x86_64")
endif()

set(LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.local/${TARGET_ARCH}")
# Prioritize local root
set(CMAKE_PREFIX_PATH "${LOCAL_ROOT};${CMAKE_PREFIX_PATH}")

message(STATUS "Looking for dependencies in: ${LOCAL_ROOT}")

# 3.1 cJSON
find_package(cJSON CONFIG REQUIRED)

# 3.2 Libuv
find_package(libuv CONFIG)
if(NOT libuv_FOUND)
    find_library(LIBUV_LIB NAMES uv uv_a libuv.a PATHS "${LOCAL_ROOT}/lib" NO_DEFAULT_PATH)
    find_path(LIBUV_INCLUDE NAMES uv.h PATHS "${LOCAL_ROOT}/include" NO_DEFAULT_PATH)
    if(LIBUV_LIB AND LIBUV_INCLUDE)
        add_library(libuv::uv STATIC IMPORTED)
        set_target_properties(libuv::uv PROPERTIES 
            IMPORTED_LOCATION "${LIBUV_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBUV_INCLUDE}"
        )
    else()
        message(FATAL_ERROR "libuv not found in local. Run ./build_vendors.sh first.")
    endif()
endif()

# 3.3 CMocka
find_package(cmocka CONFIG)
if(NOT cmocka_FOUND)
    # Fallback for manual find if config not generated
    find_library(CMOCKA_LIB NAMES cmocka cmocka-static libcmocka.a PATHS "${LOCAL_ROOT}/lib" NO_DEFAULT_PATH)
    find_path(CMOCKA_INCLUDE NAMES cmocka.h PATHS "${LOCAL_ROOT}/include" NO_DEFAULT_PATH)
    if(CMOCKA_LIB AND CMOCKA_INCLUDE)
        add_library(cmocka::cmocka STATIC IMPORTED)
        set_target_properties(cmocka::cmocka PROPERTIES 
            IMPORTED_LOCATION "${CMOCKA_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMOCKA_INCLUDE}"
        )
    endif()
endif()

# 3.4 SQLite3 (Manual)
find_library(SQLITE3_LIB NAMES sqlite3 libsqlite3.a PATHS "${LOCAL_ROOT}/lib" NO_DEFAULT_PATH)
find_path(SQLITE3_INCLUDE NAMES sqlite3.h PATHS "${LOCAL_ROOT}/include" NO_DEFAULT_PATH)
if(SQLITE3_LIB AND SQLITE3_INCLUDE)
    add_library(SQLite::SQLite3 STATIC IMPORTED)
    set_target_properties(SQLite::SQLite3 PROPERTIES
        IMPORTED_LOCATION "${SQLITE3_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${SQLITE3_INCLUDE}"
    )
else()
    message(FATAL_ERROR "sqlite3 not found in local. Run ./build_vendors.sh first.")
endif()

# ==============================================================================
# 4. Core Library
# ==============================================================================
add_library(edge_core_lib STATIC
    src/edge_version.c
    src/shmring.c
    src/algorithm_sum.c
    src/sqlite_service.c
    src/modbus_protocol.c
    # src/core_mem.c
    # src/arbiter.c
    # src/io_executor.c
    # src/driver_manager.c
)
# Modern usage: link targets, not paths
target_link_libraries(edge_core_lib PUBLIC 
    cjson 
    libuv::uv 
    SQLite::SQLite3
)
target_include_directories(edge_core_lib PUBLIC include)

# ==============================================================================
# 5. Executable
# ==============================================================================
add_executable(edge_app src/main.c)
target_link_libraries(edge_app PRIVATE edge_core_lib m pthread dl)

# ==============================================================================
# 6. Tests
# ==============================================================================
enable_testing()
add_subdirectory(tests)
