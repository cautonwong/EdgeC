# EdgeOS Platform Adaptation Layer
# ------------------------------------------------------------------------------

if(APPLE)
    # macOS placeholder
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES "linux/io_executor_linux.c")
endif()

# 改为 STATIC 库
add_library(edge_platform STATIC ${PLATFORM_SOURCES})
target_link_libraries(edge_platform PUBLIC edge_common_interface CURL::libcurl)
