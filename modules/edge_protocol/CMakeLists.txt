# modules/edge_protocol/CMakeLists.txt

set(PROTO_SOURCES
    src/can_codec.c
    src/dlt645/dlt645_codec.c
    src/dlt698/dlt698_codec.c
)

if(CONFIG_EDGEC_PROTO_MODBUS)
    list(APPEND PROTO_SOURCES src/modbus/modbus_codec.c)
endif()

if(CONFIG_EDGEC_PROTO_DLMS)
    list(APPEND PROTO_SOURCES
        src/dlms/dlms_codec.c
        src/dlms/dlms_cosem.c
        src/dlms/dlms_gbt.c
        src/dlms/dlms_hdlc.c
        src/dlms/dlms_security.c
    )
endif()

if(CONFIG_EDGEC_PROTO_IEC104)
    list(APPEND PROTO_SOURCES
        src/iec_common/iec_asdu_codec.c
        src/iec_common/iec_cp56time.c
        src/iec104/iec104_apci.c
        src/iec101/iec101_link.c
    )
endif()

# 根据 Kconfig 配置条件编译相应的工业协议栈文件以精细化构建 (以接地气的中文添加注释)
add_library(edge_protocol STATIC
    ${PROTO_SOURCES}
)

target_include_directories(edge_protocol PUBLIC include)
target_link_libraries(edge_protocol PUBLIC edge_core)

