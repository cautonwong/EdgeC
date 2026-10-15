# modules/edge_bootstrap/CMakeLists.txt
# EdgeOS Single Composition Root & Application Bootstrap Layer

add_library(edge_bootstrap STATIC
    src/edge_composition_root.c
)

target_include_directories(edge_bootstrap PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)

target_link_libraries(edge_bootstrap PUBLIC
    edge_common_interface
    edge_core
    edge_infra
)
