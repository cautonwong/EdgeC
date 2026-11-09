# modules/edge_infra_security/CMakeLists.txt
#
# Security mechanisms (license, ESAM, SOS/DPI). Kept as a separate infra module
# so edge_infra remains storage/bus/NPU and products can opt into security.
set(SEC_SOURCES
    src/license_engine.c
    src/edge_esam_driver.c
    src/edge_sos_dscp.c
)

if(CONFIG_EDGEC_SECURITY_DPI)
    list(APPEND SEC_SOURCES src/edge_guard_dpi.c)
endif()

add_library(edge_infra_security STATIC
    ${SEC_SOURCES}
)

target_include_directories(edge_infra_security PUBLIC include)
target_link_libraries(edge_infra_security PUBLIC
    edge_core
    edge_log
    edge_foundation
    edge_common_interface
)
