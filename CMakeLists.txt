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
# 3. 核心库目标 (Core Library Target)
# ==============================================================================
include_directories(include)

add_library(edge_core_lib STATIC
    src/edge_version.c
    src/core_mem.c
)

# ==============================================================================
# 4. 可执行文件目标 (Executable Target)
# ==============================================================================
add_executable(edge_app src/main.c)
target_link_libraries(edge_app PRIVATE edge_core_lib)

# ==============================================================================
# 5. 测试集成 (Testing)
# ==============================================================================
enable_testing()
add_subdirectory(tests)