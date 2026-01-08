# EdgeOS Protocol Stacks (Pure Codec Kernels)
# ------------------------------------------------------------------------------

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(PROTO_SOURCES
    "${PROTO_DIR}/dlms/dlms_codec.c"
    "${PROTO_DIR}/dlms/dlms_cosem.c"
    "${PROTO_DIR}/dlms/dlms_gbt.c"
    "${PROTO_DIR}/dlms/dlms_hdlc.c"
    "${PROTO_DIR}/dlms/dlms_security.c"
    "${PROTO_DIR}/dlt645/dlt645_codec.c"
    "${PROTO_DIR}/modbus/modbus_codec.c"
)

# 改为 STATIC 库
add_library(edge_protocols STATIC ${PROTO_SOURCES})
target_link_libraries(edge_protocols PUBLIC edge_common_interface)
target_include_directories(edge_protocols PRIVATE "${PROTO_DIR}")