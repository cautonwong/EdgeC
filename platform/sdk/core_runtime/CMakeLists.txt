# Add the sources for the core_runtime module to the parent SDK library
list(APPEND SDK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core_mem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/core_sched.c"
)
