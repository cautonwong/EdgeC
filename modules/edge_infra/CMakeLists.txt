# ==============================================================================
# EdgeOS Infrastructure & Driver Subsystem (edge_infra)
# ==============================================================================

add_library(edge_infra INTERFACE)

target_include_directories(edge_infra INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
