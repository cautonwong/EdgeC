# modules/edge_mgmt/CMakeLists.txt

find_package(OpenSSL REQUIRED)

add_library(edge_mgmt STATIC
    src/api/console_ws.c
    src/api/sniffer_ws.c
    src/auth/console_rbac.c
    src/audit/console_audit.c
    src/edge_app_mgr.c
    src/app_manifest.c
    src/edge_authz.c
    src/edge_eapp.c
    src/app_pressure.c
    src/app_secret.c
    src/app_task.c
    src/point_history.c
)

target_include_directories(edge_mgmt PUBLIC
    include
)

target_link_libraries(edge_mgmt PUBLIC
    edge_common_interface
    edge_foundation
    libuv::uv_a
)

target_link_libraries(edge_mgmt PRIVATE OpenSSL::Crypto cjson)
