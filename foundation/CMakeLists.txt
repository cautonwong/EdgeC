# foundation: pure shared code. Standard library only; no IO, no globals, no business rules.
add_library(edge_foundation STATIC
    src/iec_cp56time.c
    src/simd_cdd_codec.c
    src/multibus_bridge.c
    src/edge_tlv.c
    src/edge_app_root.c
    src/edge_launcher_spec.c
    src/contract_registry.c
    src/edge_wasm_imports.c
    src/edge_thread_port.c
)
target_include_directories(edge_foundation PUBLIC include)

# edge_log: shared zlog-backed logger. Kept as a foundation support library so
# components can log without linking the whole edge_core runtime.
add_library(edge_log STATIC
    src/edge_log.c
)
target_include_directories(edge_log PUBLIC include)
target_link_libraries(edge_log PUBLIC zlog::zlog)

# edge_math: pure matrix/DSP/control math shared by components.
if(NOT CMAKE_CROSSCOMPILING)
    find_package(GSL QUIET)
endif()
if(GSL_FOUND)
    set(MATH_IMPL src/math/linalg_gsl.c)
    set(MATH_DEFS USE_GSL)
else()
    set(MATH_IMPL src/math/linalg_builtin.c)
    set(MATH_DEFS USE_BUILTIN_MATH)
endif()

set(EDGE_MATH_SOURCES
    src/math/control_algo.c
    src/math/peak_shaving.c
    src/math/motor_algo.c
    src/math/radar_algo.c
    src/math/fmcw_dsp_neon.c
)
if(CONFIG_EDGEC_ALGO_LINALG)
    list(APPEND EDGE_MATH_SOURCES ${MATH_IMPL})
endif()
if(CONFIG_EDGEC_ALGO_DSP)
    list(APPEND EDGE_MATH_SOURCES
        src/math/windowing.c
        src/math/dsp_algo.c
    )
endif()
if(CONFIG_EDGEC_ALGO_AHRS)
    list(APPEND EDGE_MATH_SOURCES src/math/quat_math.c)
endif()

add_library(edge_math STATIC ${EDGE_MATH_SOURCES})
target_include_directories(edge_math PUBLIC include)
target_link_libraries(edge_math PUBLIC m)
target_compile_definitions(edge_math PRIVATE ${MATH_DEFS})
if(GSL_FOUND)
    target_link_libraries(edge_math PRIVATE GSL::gsl GSL::gslcblas)
endif()

# edge_netbuf: shared industrial buffer pool. Kept separate from the pure
# foundation because its mutex/cond implementation comes from libuv.
add_library(edge_netbuf STATIC
    src/net_buf.c
)
target_include_directories(edge_netbuf PUBLIC include)
target_link_libraries(edge_netbuf PUBLIC libuv::uv_a)

# edge_crypto: consumer-owned crypto ops registry (no backend implementation).
add_library(edge_crypto STATIC
    src/hal_crypto.c
)
target_include_directories(edge_crypto PUBLIC include)

# edge_codec: pure byte-level protocol codecs. No logging, no DataHub, no IO.
set(EDGE_CODEC_SOURCES
    src/protocol/can_codec.c
    src/protocol/dlt645_codec.c
    src/protocol/dlt698_codec.c
    src/protocol_converter.c
    src/edge_uns_engine.c
)
if(CONFIG_EDGEC_PROTO_MODBUS)
    list(APPEND EDGE_CODEC_SOURCES src/protocol/modbus_codec.c)
endif()
if(CONFIG_EDGEC_PROTO_DLMS)
    list(APPEND EDGE_CODEC_SOURCES
        src/protocol/dlms_codec.c
        src/protocol/dlms_cosem.c
        src/protocol/dlms_gbt.c
        src/protocol/dlms_hdlc.c
        src/protocol/dlms_security.c
    )
endif()
if(CONFIG_EDGEC_PROTO_IEC104)
    list(APPEND EDGE_CODEC_SOURCES
        src/protocol/iec_asdu_codec.c
        src/protocol/iec104_apci.c
        src/protocol/iec101_link.c
    )
endif()
add_library(edge_codec STATIC ${EDGE_CODEC_SOURCES})
target_include_directories(edge_codec PUBLIC include)
target_link_libraries(edge_codec PUBLIC edge_foundation edge_netbuf edge_crypto)
