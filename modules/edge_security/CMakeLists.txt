# modules/edge_security/CMakeLists.txt

set(SECURITY_SOURCES
    src/license_engine.c
    src/edge_sos_dscp.c
    src/edge_acl.c
    src/dfu_dualbank.c
    src/ebpf_guard.c
    src/edge_spiffe_auth.c
    src/edge_esam_driver.c
)

# 根据 Kconfig 条件加载 DPI 深度报文检测防火墙模块 (以接地气的中文添加注释)
if(CONFIG_EDGEC_SECURITY_DPI)
    list(APPEND SECURITY_SOURCES src/edge_guard_dpi.c)
endif()

# 根据 Kconfig 条件加载防篡改加密签名审计日志模块
if(CONFIG_EDGEC_SECURITY_SIGNED_LOGS)
    list(APPEND SECURITY_SOURCES src/secure_syslog_exporter.c)
endif()

add_library(edge_security STATIC
    ${SECURITY_SOURCES}
)

target_include_directories(edge_security PUBLIC include)
target_link_libraries(edge_security PUBLIC edge_core)
# target_link_libraries(edge_security PRIVATE mbedTLS::mbedtls)
