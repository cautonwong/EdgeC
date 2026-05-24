# modules/edge_security/CMakeLists.txt

add_library(edge_security STATIC
    src/license_engine.c
    src/edge_guard_dpi.c
    src/secure_syslog_exporter.c
)

target_include_directories(edge_security PUBLIC include)
target_link_libraries(edge_security PUBLIC edge_core)
# target_link_libraries(edge_security PRIVATE mbedTLS::mbedtls)
