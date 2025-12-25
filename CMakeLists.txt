cmake_minimum_required(VERSION 3.15)
project(EdgeOS_Seed VERSION 0.1.0 LANGUAGES C)

# ==============================================================================
# 1. 全局配置 (Global Configuration)
# ==============================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 引入自定义 CMake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)

# ==============================================================================
# 2. 编译选项 (Compiler Options)
# ==============================================================================
add_compile_options(
    -Wall -Wextra -Wshadow -Wformat=2 -Wunused -Wconversion
    $<$<CONFIG:Release>:-Werror> 
)

# Debug 模式下开启 ASan (内存检测)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_sanitizers(address undefined)
endif()

# ==============================================================================
# 3. Vendor 依赖 (Vendor Dependencies)
# ==============================================================================
# ==============================================================================
# 3. Vendor 依赖 (Vendor Dependencies) - 从 .local 查找预编译库
# ==============================================================================
set(VENDOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.local")

# 强制 CMake 只在指定路径下查找库和头文件，避免使用系统路径
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# UV
find_library(UV_LIB NAMES uv_a uv PATHS
    "${VENDOR_ROOT}/linux-x86_64/lib"
    "${VENDOR_ROOT}/linux-aarch64/lib"
    "${VENDOR_ROOT}/linux-arm64/lib"
)
find_path(UV_INCLUDE NAMES uv.h PATHS
    "${VENDOR_ROOT}/linux-x86_64/include"
    "${VENDOR_ROOT}/linux-aarch64/include"
    "${VENDOR_ROOT}/linux-arm64/include"
)
if(NOT UV_LIB OR NOT UV_INCLUDE)
    message(FATAL_ERROR "libuv not found in ${VENDOR_ROOT}")
endif()

# CMocka
find_library(CMOCKA_LIB NAMES cmocka-static cmocka PATHS
    "${VENDOR_ROOT}/linux-x86_64/lib"
    "${VENDOR_ROOT}/linux-aarch64/lib"
    "${VENDOR_ROOT}/linux-arm64/lib"
)
find_path(CMOCKA_INCLUDE NAMES cmocka.h PATHS
    "${VENDOR_ROOT}/linux-x86_64/include"
    "${VENDOR_ROOT}/linux-aarch64/include"
    "${VENDOR_ROOT}/linux-arm64/include"
)
if(NOT CMOCKA_LIB OR NOT CMOCKA_INCLUDE)
    message(FATAL_ERROR "cmocka not found in ${VENDOR_ROOT}")
endif()

# SQLite3
find_library(SQLITE3_LIB NAMES sqlite3 PATHS
    "${VENDOR_ROOT}/linux-x86_64/lib"
    "${VENDOR_ROOT}/linux-aarch64/lib"
    "${VENDOR_ROOT}/linux-arm64/lib"
)
find_path(SQLITE3_INCLUDE NAMES sqlite3.h PATHS
    "${VENDOR_ROOT}/linux-x86_64/include"
    "${VENDOR_ROOT}/linux-aarch64/include"
    "${VENDOR_ROOT}/linux-arm64/include"
)
if(NOT SQLITE3_LIB OR NOT SQLITE3_INCLUDE)
    message(FATAL_ERROR "sqlite3 not found in ${VENDOR_ROOT}")
endif()

# cJSON (Used by project, assuming it's built to .local as well by build_vendors.sh)
find_library(CJSON_LIB NAMES cjson PATHS
    "${VENDOR_ROOT}/linux-x86_64/lib"
    "${VENDOR_ROOT}/linux-aarch64/lib"
    "${VENDOR_ROOT}/linux-arm64/lib"
)
find_path(CJSON_INCLUDE NAMES cJSON.h PATHS
    "${VENDOR_ROOT}/linux-x86_64/include/cjson"
    "${VENDOR_ROOT}/linux-x86_64/include" # Keep this in case other platforms install differently
    "${VENDOR_ROOT}/linux-aarch64/include/cjson"
    "${VENDOR_ROOT}/linux-aarch64/include"
    "${VENDOR_ROOT}/linux-arm64/include/cjson"
    "${VENDOR_ROOT}/linux-arm64/include"
)
if(NOT CJSON_LIB OR NOT CJSON_INCLUDE)
    message(FATAL_ERROR "cJSON not found in ${VENDOR_ROOT}")
endif()

# ==============================================================================
# 4. 核心库目标 (Core Library Target)
# ==============================================================================
include_directories(include
    ${UV_INCLUDE}
    ${CMOCKA_INCLUDE}
    ${SQLITE3_INCLUDE}
    ${CJSON_INCLUDE}
)

add_library(edge_core_lib STATIC
    src/edge_version.c
    src/core_mem.c
    src/arbiter.c
    src/io_executor.c
    src/shm_ring.c
    src/algorithm_sum.c
    src/sqlite_service.c
    src/modbus_protocol.c
    src/driver_manager.c
)
target_link_libraries(edge_core_lib PRIVATE ${SQLITE3_LIB})
# cJSON is not used by the core lib yet, but we can add it here when needed
# target_link_libraries(edge_core_lib PUBLIC cjson)


# ==============================================================================
# 5. 可执行文件目标 (Executable Target)
# ==============================================================================
add_executable(edge_app src/main.c)
target_link_libraries(edge_app PRIVATE edge_core_lib ${UV_LIB} m)

# ==============================================================================
# 6. 测试集成 (Testing)
# ==============================================================================
enable_testing()
add_subdirectory(tests)