# modules/edge_mgmt/CMakeLists.txt

add_library(edge_mgmt STATIC
    src/api/console_ws.c
    src/api/sniffer_ws.c
    src/auth/console_rbac.c
    src/audit/console_audit.c
    src/edge_app_mgr.c
)

target_include_directories(edge_mgmt PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_core/include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)

target_link_libraries(edge_mgmt PUBLIC
    edge_common_interface
    edge_core
    edge_infra
)
