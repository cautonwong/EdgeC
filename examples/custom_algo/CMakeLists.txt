# Example: Custom SOC Filter Algorithm Standalone Library
add_library(custom_soc_filter STATIC
    src/custom_soc_filter.c
)

target_include_directories(custom_soc_filter PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)

target_link_libraries(custom_soc_filter PRIVATE
    edge_common_interface
    m
)
