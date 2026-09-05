# modules/edge_mgmt/CMakeLists.txt

add_library(edge_mgmt STATIC
    src/api/console_ws.c
)

target_include_directories(edge_mgmt PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
)

target_link_libraries(edge_mgmt PUBLIC
    edge_common_interface
    edge_core
)
