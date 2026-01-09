# EdgeOS Protocol Stacks (Pure Codec Kernels)
# ------------------------------------------------------------------------------

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(PROTO_SOURCES
    "${PROTO_DIR}/dlms/dlms_codec.c"
    "${PROTO_DIR}/dlms/dlms_cosem.c"
    "${PROTO_DIR}/dlms/dlms_gbt.c"
    "${PROTO_DIR}/dlms/dlms_hdlc.c"
    "${PROTO_DIR}/dlms/dlms_security.c"
    "${PROTO_DIR}/iec_common/iec_asdu_codec.c"
    "${PROTO_DIR}/iec104/iec104_apci.c"
    "${PROTO_DIR}/iec101/iec101_link.c"
)

# 改为 STATIC 库
add_library(edge_protocols STATIC ${PROTO_SOURCES})
target_link_libraries(edge_protocols PUBLIC edge_common_interface)
target_include_directories(edge_protocols PUBLIC "${PROTO_DIR}")
