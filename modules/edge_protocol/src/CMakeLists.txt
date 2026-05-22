# EdgeOS Protocol Stacks (Pure Codec Kernels)
# ------------------------------------------------------------------------------

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(PROTO_SOURCES
    "${PROTO_DIR}/modbus/modbus_codec.c"
    "${PROTO_DIR}/dlt645/dlt645_codec.c"
    "${PROTO_DIR}/dlt698/dlt698_codec.c"
    "${PROTO_DIR}/can_codec.c"
    "${PROTO_DIR}/dlms/dlms_codec.c"
    "${PROTO_DIR}/dlms/dlms_cosem.c"
    "${PROTO_DIR}/dlms/dlms_gbt.c"
    "${PROTO_DIR}/dlms/dlms_hdlc.c"
    "${PROTO_DIR}/dlms/dlms_security.c"
    "${PROTO_DIR}/iec_common/iec_asdu_codec.c"
    "${PROTO_DIR}/iec_common/iec_cp56time.c"
    "${PROTO_DIR}/iec104/iec104_apci.c"
    "${PROTO_DIR}/iec101/iec101_link.c"
)

# 使用 STATIC 库，方便 Plane 链接
add_library(edge_protocols STATIC ${PROTO_SOURCES})

# 显式包含核心头文件目录
target_include_directories(edge_protocols PUBLIC 
    "${PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

# 链接接口库以获得编译定义
target_link_libraries(edge_protocols PUBLIC edge_common_interface)
