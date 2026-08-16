# modules/edge_io/CMakeLists.txt

file(GLOB ENGINE_SOURCES src/engines/*.c)
file(GLOB REQUEST_SOURCES src/requests/*.c)
file(GLOB DRIVER_SOURCES src/driver_*.c)

set(IO_SOURCES
    src/bus_orchestrator.c
    src/driver_manager.c
    src/driver_hotplug.c
    src/passthrough_mgr.c
    src/protocol_bridge.c
    src/cloud_sync_service.c
    src/edge_agent.c
    src/rest_server.c
    src/web_server.c
    src/ws_proto.c
    src/profile_loader.c
    src/session_manager.c
    # 命名不符合 driver_*.c 模式，需要显式列出 (以接地气的中文添加注释)
    src/edge_api.c
    src/edge_bsp_hal.c
    src/adc_driver.c
    src/modbus_rtu_driver.c
    src/modbus_tcp_driver.c
    src/driver_dlt698.c
    ${CMAKE_SOURCE_DIR}/third-party/open62541/open62541.c
    ${ENGINE_SOURCES}
    ${REQUEST_SOURCES}
    ${DRIVER_SOURCES}
    # 服务层与调度层代码
    src/northbound_mgr.c
    src/sched_engine.c
    src/sched_port.c
    src/io_executor_linux.c
)

# 根据 Kconfig 条件加载 Store-and-Forward (SnF) 断线缓存与可靠传输机制 (以接地气的中文添加注释)
if(CONFIG_EDGEC_SNF)
    list(APPEND IO_SOURCES src/snf_manager.c)
endif()

# 根据 Kconfig 条件加载 Periodic Persistence 定时周期性持久化同步服务
if(CONFIG_EDGEC_PERSISTENCE)
    list(APPEND IO_SOURCES src/persistence_mgr.c)
endif()

add_library(edge_io STATIC
    ${IO_SOURCES}
)

target_include_directories(edge_io PUBLIC include src/requests ${CMAKE_SOURCE_DIR}/third-party/open62541)
target_include_directories(edge_io PRIVATE src/engines)

# edge_io: 包含 SnF 断网落盘缓存 (snf_manager.c) 与 MQTT 南北向执行器 (edge_agent.c, io_executor_linux.c) 显式适配器
target_link_libraries(edge_io PUBLIC edge_infra edge_core edge_protocol edge_stream edge_security edge_common_interface cyclonedds)
target_link_libraries(edge_io PRIVATE libuv::uv_a SQLite::SQLite3 PahoMQTT::PahoMQTT cjson CURL::libcurl)


