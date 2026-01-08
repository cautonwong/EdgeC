# EdgeOS Protocol Stacks (Pure Codec Kernels)
# ------------------------------------------------------------------------------

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(PROTO_SOURCES
    "${PROTO_DIR}/dlms/dlms_codec.c"
    "${PROTO_DIR}/dlms/dlms_cosem.c"
    "${PROTO_DIR}/dlms/dlms_gbt.c"
    "${PROTO_DIR}/dlms/dlms_hdlc.c"
    "${PROTO_DIR}/dlms/dlms_security.c"
)

# 使用 OBJECT 库模式
add_library(edge_protocols_obj OBJECT ${PROTO_SOURCES})
target_include_directories(edge_protocols_obj PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${PROTO_DIR}"
    "${LOCAL_ROOT}/include"
)
target_compile_definitions(edge_protocols_obj PRIVATE _DEFAULT_SOURCE)

# 提供 INTERFACE 库
add_library(edge_protocols INTERFACE)
target_sources(edge_protocols INTERFACE $<TARGET_OBJECTS:edge_protocols_obj>)
