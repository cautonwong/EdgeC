add_library(port_impl STATIC src/adapter.c)
target_link_libraries(port_impl PUBLIC alpha foundation_base)
