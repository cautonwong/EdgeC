# EdgeOS Platform Adaptation Layer
# ------------------------------------------------------------------------------

if(APPLE)
    # macOS placeholder
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES "linux/io_executor_linux.c")
endif()

# 使用 OBJECT 库以物理消除链接顺序问题
add_library(edge_platform_obj OBJECT ${PLATFORM_SOURCES})
target_include_directories(edge_platform_obj PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${LOCAL_ROOT}/include"
)
target_compile_definitions(edge_platform_obj PRIVATE _DEFAULT_SOURCE)

# 仍然保留一个 INTERFACE 库供依赖
add_library(edge_platform INTERFACE)
target_sources(edge_platform INTERFACE $<TARGET_OBJECTS:edge_platform_obj>)