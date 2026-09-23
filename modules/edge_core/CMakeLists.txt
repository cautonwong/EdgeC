# modules/edge_core/CMakeLists.txt

add_library(edge_core STATIC
    src/edge_version.c
    src/shmring.c
    src/datahub.c
    src/datahub_shm.c
    src/datahub_router.c
    src/net_buf.c
    src/edge_log.c
    src/contract_registry.c
    src/arbiter.c
    src/edge_params.c
    src/edge_os_zephyr.c
    src/edge_service.c
    src/edge_app_mgr.c
    src/edge_mesh.c
    src/edge_metrics.c      # 💡 [ADR-0001] 系统 Metrics 通道
    src/edge_tlv.c
    src/timesync.c
    src/shm_waitset.c
    src/thermal_throttle.c
    src/device_twin.c
    src/resilient_buffer.c
    src/wasm_host_bridge.c
    src/timeseries_compressor.c
    src/edge_port.c
    src/edge_composition_root.c
)

# PUBLIC: 链接 edge_core 的 Target 也能 include 这些头文件
target_include_directories(edge_core PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/modules/edge_infra/include
)

# PRIVATE: 依赖项
target_link_libraries(edge_core PUBLIC edge_common_interface edge_infra)
target_link_libraries(edge_core PRIVATE libuv::uv_a cjson zlog::zlog)

